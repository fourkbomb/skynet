/*
 * Game.c
 * Implementation of Game.h for Knowledge Island
 *
 * Copyright 2015 Simon Shields, Harrison Shoebridge, Julian Tu and James Ye
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Game.h"

#define NUM_DISCIPLINE 6
#define MAP_ARC_HEIGHT 21
#define MAP_ARC_WIDTH 11
#define MAP_VERTEX_HEIGHT 11
#define MAP_VERTEX_WIDTH 6
#define MAP_REGION_HEIGHT 9
#define MAP_REGION_WIDTH 5
#define VERTEX_SIZE sizeof(int) * MAP_VERTEX_HEIGHT * MAP_VERTEX_HEIGHT
#define ARC_SIZE sizeof(int) * MAP_ARC_HEIGHT * MAP_ARC_WIDTH

#define STARTING_TURN_NUM -1
#define STARTING_KPI 20
#define STARTING_ARCS 0
#define STARTING_CAMPUSES 2
#define STARTING_GO8S 0
#define STARTING_PATENTS 0
#define STARTING_PUBLICATIONS 0
#define STARTING_THD 0
#define STARTING_BPS_BPN 3
#define STARTING_MJ_MTV_MMONEY 1
#define STARTING_EXCHANGE_RATE 3

typedef struct _region {
    int discipline;
    int diceValue;
} region;

typedef struct _coord {
    int x;
    int y;
    char lastMove; // optional
} coord;

typedef struct _game {
    // basic state information
    int turnNumber;
    int whoseTurn;
    int mostARCgrants;
    int mostPublications;
    int numGO8s;

    // scores for each university
    int kpi[NUM_UNIS];
    int arcGrants[NUM_UNIS];
    int campuses[NUM_UNIS];
    int groupOfEights[NUM_UNIS];
    int patents[NUM_UNIS];
    int publications[NUM_UNIS];

    // each discipline for each university
    int students[NUM_UNIS][NUM_DISCIPLINE];
    int exchangeRates[NUM_UNIS][NUM_DISCIPLINE];

    // board layout settings
    int *boardTileDisciplines;
    int *boardTileDice;

    // the board is represented as a flattened grid
    /* for example:
    *         0123456789A
    *         0 1 2 3 4 5
    *          0 1 2 3 4
    *  0 0       R*-*
    *  1   0      |x|
    *  2 1      *-* *-*
    *  3   1    |x| |x|
    *  4 2    *-* *-* *-*P
    *  5   2  |x| |x| |x|
    *  6 3   T* *-* *-* *
    *  7   3  | |x| |x| |
    *  8 4    *-* *-* *-*
    *  9   4  |x| |x| |x|
    * 10 5    * *-* *-* *
    * 11   5  | |x| |x| |
    * 12 6    *-* *-* *-*
    * 13   6  |x| |x| |x|
    * 14 7    * *-* *-* *T
    * 15   7  | |x| |x| |
    * 16 8   P*-* *-* *-*
    * 17   8    | |x| |
    * 18 9      *-* *-*
    * 19          | |
    * 20 10       *-*R
    */
    // vertices, arcs and regions are separate but map to each other
    // (y axis then x axis)
    int vertices[MAP_VERTEX_HEIGHT][MAP_VERTEX_WIDTH];
    int arcs[MAP_ARC_HEIGHT][MAP_ARC_WIDTH];
    region regions[MAP_REGION_HEIGHT][MAP_REGION_WIDTH];
} game;

static int isValidRegion(int x, int y);
static int isValidVertex(int x, int y);
static int isValidARC(int x, int y);
static int isValidAction(Game g, action a);
static int hasMostARCgrants(Game g, int player);
static int ownsARCadjacentTo(Game g, coord arc, int player);
static int ownsVertexBorderingARC(Game g, coord arc, int player);
static int ownsARCborderingVertex(Game g, coord vertex, int player);
static int isCampusAdjacent(Game g, coord vertex);
static void updateKPI(Game g, int player, int actionCode);
static void updateArc(Game g, int x, int y, int newValue);
static void updateVertex(Game g, int x, int y, int newValue);
static void updateExchangeRate(Game g, coord vertex, int player);
static region getRegionForCoordinates(Game g, int x, int y);
static coord getVertexCoordinateFromPath(path p);
static coord getCoordinateFromPath(path p, int getArcCoord);
static coord getARCCoordinateFromPath(path p);

#ifdef DODGY_MAIN
int main(void) {
    coord c = getVertexCoordinateFromPath("RRLRLLLRLLL");
    assert(c.x == 2 && c.y == 3);
    return 0;
}
#endif

// Game functions implementing Game.h
Game newGame(int discipline[], int dice[]) {
    game *g = malloc(sizeof(game));
    g->turnNumber = STARTING_TURN_NUM;
    g->whoseTurn = NO_ONE;
    g->mostARCgrants = NO_ONE;
    g->mostPublications = NO_ONE;
    g->numGO8s = 0;
    g->boardTileDisciplines = discipline;
    g->boardTileDice = dice;

    int player = UNI_A - 1;
    while (player < NUM_UNIS) {
        g->kpi[player] = STARTING_KPI;
        g->arcGrants[player] = STARTING_ARCS;
        g->campuses[player] = STARTING_CAMPUSES;
        g->groupOfEights[player] = STARTING_GO8S;
        g->patents[player] = STARTING_PATENTS;
        g->publications[player] = STARTING_PUBLICATIONS;

        int student = STUDENT_THD;
        while (student <= STUDENT_MMONEY) {
            if (student == STUDENT_THD) {
                g->students[player][student] = STARTING_THD;
            } else if (student == STUDENT_BPS ||
                    student == STUDENT_BQN) {
                g->students[player][student] = STARTING_BPS_BPN;
            } else if (student == STUDENT_MJ ||
                    student == STUDENT_MTV ||
                    student == STUDENT_MMONEY) {
                g->students[player][student] = STARTING_MJ_MTV_MMONEY;
            }
            g->exchangeRates[player][student] = STARTING_EXCHANGE_RATE;
            student++;
        }
        player++;
    }

    int x = 0;
    int arrayIndex = 0;
    while (x < 5) {
        int y = 0;
        while (y < 9) {
            if (isValidRegion(x,y)) {
                region r;
                r.discipline = discipline[arrayIndex];
                r.diceValue = dice[arrayIndex];
                arrayIndex++;
                // DEBUG: printf("+1 ");
                g->regions[y][x] = r;
            }
            y++;
        }
        x++;
    }

    memset(g->vertices, VACANT_VERTEX, VERTEX_SIZE);
    memset(g->arcs, VACANT_ARC, ARC_SIZE);

    // player 1 starting points
    updateVertex(g, 2, 0, CAMPUS_A);
    updateVertex(g, 3, 10, CAMPUS_A);
    // player 2 starting points 0,3 5,7 0,7 5,13
    updateVertex(g, 0, 3, CAMPUS_B);
    updateVertex(g, 5, 7, CAMPUS_B);
    // player 3 starting points 0,8 5,2 1,16 9,4
    updateVertex(g, 0, 8, CAMPUS_C);
    updateVertex(g, 5, 2, CAMPUS_C);

    return g;
}

void disposeGame(Game g) {
    free(g);
    g = NULL;
}

void makeAction(Game g, action a) {
    if (!isValidAction(g, a)) {
        printf("ERROR: INVALID ACTION\n");
    }
    int actionType = a.actionCode;
    int currentPlayer = getWhoseTurn(g);
    if (actionType == BUILD_CAMPUS) {
        g->students[currentPlayer-1][STUDENT_BQN]--;
        g->students[currentPlayer-1][STUDENT_BPS]--;
        g->students[currentPlayer-1][STUDENT_MJ]--;
        g->students[currentPlayer-1][STUDENT_MTV]--;

        coord pos = getVertexCoordinateFromPath(a.destination);
        updateVertex(g, pos.x, pos.y, currentPlayer);
        g->campuses[currentPlayer-1]++;
        updateExchangeRate(g, pos, currentPlayer);
        updateKPI(g, currentPlayer, BUILD_CAMPUS);
    } else if (actionType == BUILD_GO8) {
        g->students[currentPlayer-1][STUDENT_MJ] -= 2;
        g->students[currentPlayer-1][STUDENT_MMONEY] -= 3;

        coord pos = getVertexCoordinateFromPath(a.destination);
        updateVertex(g, pos.x, pos.y, currentPlayer+3);
        g->groupOfEights[currentPlayer-1]++;
        g->campuses[currentPlayer-1]--;
        g->numGO8s++;
        updateKPI(g, currentPlayer, BUILD_GO8);
    } else if (actionType == OBTAIN_ARC) {
        g->students[currentPlayer-1][STUDENT_BPS]--;
        g->students[currentPlayer-1][STUDENT_BQN]--;

        coord path = getARCCoordinateFromPath(a.destination);
        updateArc(g, path.x, path.y, currentPlayer);
        g->arcGrants[currentPlayer-1]++;
        updateKPI(g, currentPlayer, OBTAIN_ARC);
    } else if (actionType == OBTAIN_PUBLICATION ||
            actionType == OBTAIN_IP_PATENT) {
        g->students[currentPlayer-1][STUDENT_MJ]--;
        g->students[currentPlayer-1][STUDENT_MTV]--;
        g->students[currentPlayer-1][STUDENT_MMONEY]--;

        if (actionType == OBTAIN_PUBLICATION) {
            g->publications[currentPlayer-1]++;
            updateKPI(g, currentPlayer, OBTAIN_PUBLICATION);
        } else { // actionType == OBTAIN_IP_PATENT
            g->patents[currentPlayer-1]++;
            updateKPI(g, currentPlayer, OBTAIN_IP_PATENT);
        }
    } else if (actionType == RETRAIN_STUDENTS) {
        int from = a.disciplineFrom;
        int to = a.disciplineTo;
        g->students[currentPlayer-1][from] -= getExchangeRate(g,
                currentPlayer, from, to);
        g->students[currentPlayer-1][to]++;
    }
}

void throwDice(Game g, int diceScore) {
    int y = 0;
    while (y < MAP_REGION_HEIGHT) {
        int x = 0;
        while (x < MAP_REGION_WIDTH) {
            region r = getRegionForCoordinates(g, x, y);
            if (r.diceValue == diceScore) {
                // commence calculation
                // adjacent vertexes are
                // x, x+1
                // y, y+1, y+2
                int yInc = 0;
                while (yInc < 3) {
                    int xInc = 0;
                    while (xInc < 2) {
                        if (g->vertices[y+yInc][x+xInc] !=
                                VACANT_VERTEX) {
                            int owner = g->vertices[y+yInc][x+xInc];
                            if (owner > 3) { // GO8
                                g->students[owner-4][r.discipline] += 2;
                            } else { // normal campus
                                g->students[owner-1][r.discipline]++;
                            }
                        }
                        xInc++;
                    }
                    yInc++;
                }
            }
            x++;
        }
        y++;
    }
    if (diceScore == 7) {
        int uni = UNI_A;
        while (uni <= UNI_C) {
            g->students[uni-1][STUDENT_THD] +=
                    g->students[uni-1][STUDENT_MTV];
            g->students[uni-1][STUDENT_THD] +=
                    g->students[uni-1][STUDENT_MMONEY];
            g->students[uni-1][STUDENT_MTV] = 0;
            g->students[uni-1][STUDENT_MMONEY] = 0;

            uni++;
        }
    }
    g->turnNumber++;
    g->whoseTurn++;
    if (g->whoseTurn > UNI_C) {
        g->whoseTurn = UNI_A;
    }
}

// game-wide getter functions
int getDiscipline(Game g, int regionID) {
    int get = 0;
    if (0 <= regionID && regionID <= NUM_REGIONS) {
        get = g->boardTileDisciplines[regionID];
    }
    return get;
}

int getDiceValue(Game g, int regionID) {
    int get = 0;
    if (0 <= regionID && regionID <= NUM_REGIONS) {
        get = g->boardTileDice[regionID];
    }
    return get;
}

int getMostARCs(Game g) {
    return g->mostARCgrants;
}

int getMostPublications(Game g) {
    return g->mostPublications;
}

int getTurnNumber(Game g) {
    return g->turnNumber;
}

int getWhoseTurn(Game g) {
    return g->whoseTurn;
}

int getCampus(Game g, path pathToVertex) {
    int get = 0;
    coord vertex = getVertexCoordinateFromPath(pathToVertex);
    if (0 <= vertex.y && vertex.y <= MAP_VERTEX_HEIGHT &&
            0 <= vertex.x && vertex.x <= MAP_VERTEX_WIDTH) {
        get = g->vertices[vertex.y][vertex.x];
    }
    return get;
}

int getARC(Game g, path pathToEdge) {
    int get = 0;
    coord arc = getARCCoordinateFromPath(pathToEdge);
    if (0 <= arc.y && arc.y <= MAP_ARC_HEIGHT &&
            0 <= arc.x && arc.x <= MAP_ARC_WIDTH) {
        get = g->arcs[arc.y][arc.x];
    }
    return get;
}

// returns TRUE if action 'a' is legal for current player.
// always FALSE in terra nullius
int isLegalAction(Game g, action a) {
    int isLegal = FALSE;
    int player = getWhoseTurn(g);
    //printf("I'm inside isLegalAction\n");
    if (getTurnNumber(g) != -1) {
        //printf("turn number ok\n");
        int actionType = a.actionCode;
        if (actionType == PASS) {
            isLegal = TRUE;
        } else if (actionType == BUILD_CAMPUS) {
            coord vertex = getVertexCoordinateFromPath(a.destination);

            if (vertex.x >= 0 && vertex.y >= 0) {
                // check there's no campus there and the player has an
                // adjacent arc
                if (getCampus(g, a.destination) == VACANT_VERTEX &&
                        ownsARCborderingVertex(g, vertex, player)) {
                    // and the player has the right resources
                    if (getStudents(g, player, STUDENT_BQN) >= 1 &&
                            getStudents(g, player, STUDENT_BPS) >= 1 &&
                            getStudents(g, player, STUDENT_MJ) >= 1 &&
                            getStudents(g, player, STUDENT_MTV) >= 1) {
                        // and there's not a campus adjacent
                        if (!isCampusAdjacent(g, vertex)) {
                            isLegal = TRUE;
                        }
                    }
                }
            }
        } else if (actionType == BUILD_GO8) {
            coord vertex = getVertexCoordinateFromPath(a.destination);

            if (vertex.x >= 0 && vertex.y >= 0) {
                int campusType = getCampus(g, a.destination);
                // check there is a campus there and there are less than
                // 8 existing GO8 campuses
                if (campusType == player && g->numGO8s < 8) {
                    // check the player has the right resources
                    if (getStudents(g, player, STUDENT_MJ) >= 2 &&
                            getStudents(g, player, STUDENT_MMONEY) >= 3) {
                        isLegal = TRUE;
                    }
                }
            }
        } else if (actionType == OBTAIN_ARC) {
            //printf("I AM ABOUT TO RUN THIS THING\n");
            coord arc = getARCCoordinateFromPath(a.destination);
            //printf("testing coords...\n");
            if (arc.x >= 0 && arc.y >= 0) {
                // the player has an arc leading to this pointor a
                // campus adjacent to the arc and that the arc is not
                // occupied
                #ifdef DEBUG
                printf("coords are ok - %d, %d\n", arc.x, arc.y);
                #endif
                if ((ownsARCadjacentTo(g, arc, player) ||
                        ownsVertexBorderingARC(g, arc, player))) {
                    #ifdef DEBUG
                    printf("adjacents ok\n");
                    #endif
                    if (getARC(g, a.destination) == VACANT_ARC) {
                        #ifdef DEBUG
                        printf("arc is vacant\n");
                        #endif
                        // check the player has enough students
                        if (getStudents(g, player, STUDENT_BQN) >= 1 &&
                                getStudents(g, player, STUDENT_BPS) >=
                                1) {
                            #ifdef DEBUG
                            printf("students are ok\n");
                            #endif
                            isLegal = TRUE;
                        }
                    } /*else {
                        printf("arc is not vacant: occupied by %d\n",
                                getARC(g, a.destination));
                    }*/
                }
            }
        } else if (actionType == START_SPINOFF) {
            if (getStudents(g, player, STUDENT_MMONEY) >= 1 &&
                    getStudents(g, player, STUDENT_MTV) >= 1 &&
                    getStudents(g, player, STUDENT_MJ) >= 1) {
                isLegal = TRUE;
            }
        } else if (actionType == RETRAIN_STUDENTS) {
            int from = a.disciplineFrom;
            int to = a.disciplineTo;
            if (STUDENT_THD <= from && from <= STUDENT_MMONEY &&
                    STUDENT_THD <= to && to <= STUDENT_MMONEY) {
                if (from != STUDENT_THD &&
                        getStudents(g, player, from) >=
                        getExchangeRate(g, player, from, to)) {
                    isLegal = TRUE;
                }
            }
        }
    }
    return isLegal;
}

// player getter functions

int getKPIpoints(Game g, int player) {
    int get = 0;
    if (UNI_A <= player && player <= UNI_C) {
        get = g->kpi[player-1];
    }
    return get;
}

int getARCs(Game g, int player) {
    int get = 0;
    if (UNI_A <= player && player <= UNI_C) {
        get = g->arcGrants[player-1];
    }
    return get;
}

int getGO8s(Game g, int player) {
    int get = 0;
    if (UNI_A <= player && player <= UNI_C) {
        get = g->groupOfEights[player-1];
    }
    return get;
}

int getCampuses(Game g, int player) {
    int get = 0;
    if (UNI_A <= player && player <= UNI_C) {
        get = g->campuses[player-1];
    }
    return get;
}

int getIPs(Game g, int player) {
    int get = 0;
    if (UNI_A <= player && player <= UNI_C) {
        get = g->patents[player-1];
    }
    return get;
}

int getPublications(Game g, int player) {
    int get = 0;
    if (UNI_A <= player && player <= UNI_C) {
        get = g->publications[player-1];
    }
    return get;
}

int getStudents(Game g, int player, int discipline) {
    int get = 0;
    if (UNI_A <= player && player <= UNI_C) {
        if (STUDENT_THD <= discipline && discipline <= NUM_DISCIPLINE) {
            get = g->students[player-1][discipline];
        }
    }
    return get;
}

// returns how many students of type disciplineFrom are needed
// to produce one student of disciplineTo
int getExchangeRate (Game g, int player, int disciplineFrom,
        int disciplineTo) {
    return g->exchangeRates[player-1][disciplineFrom];
}

// "private" functions (sick OO C)
static int isValidRegion(int x, int y) {
    // needs documentation
    int valid = TRUE;
    if (y > 8 || y < 0 || x < 0 || x > 4) {
        valid = FALSE;
    } else if (y == 0 && x != 2) {
        valid = FALSE;
    } else if ((y == 1 || y == 8) && (x == 0 || x == 4)) {
        valid = FALSE;
    } else if (x % 2 != y % 2) {
        valid = FALSE;
    }
    return valid;
}

// determines if the player has the most ARC grants. mostARCgrants in
// struct _game is NOT used NOR updated
static int hasMostARCgrants(Game g, int player) {
    int mostARCgrants = TRUE;
    int uni = UNI_A;
    while (uni <= UNI_C) {
        if (uni != player) {
            if (g->arcGrants[uni-1] >= g->arcGrants[player-1]) {
                mostARCgrants = FALSE;
            }
        }
        uni++;
    }
    return mostARCgrants;
}

// computes who has the most publications. mostPublications in
// struct _game is NOT used NOR updated
static int hasMostPublications(Game g, int player) {
    int mostPublications = TRUE;
    int uni = UNI_A;
    while (uni <= UNI_C) {
        if (uni != player) {
            if (g->publications[uni-1] >= g->publications[player-1]) {
                mostPublications = FALSE;
            }
        }
        uni++;
    }
    return mostPublications;
}

static region getRegionForCoordinates(Game g, int x, int y) {
    return g->regions[y][x];
}

// updateKPI must run AFTER the action has been successfully executed
// side effects: updateKPI also updates mostARCgrants and
// mostPublications where relevant
static void updateKPI(Game g, int player, int actionCode) {
    int kpi = 0;
    if (actionCode == BUILD_CAMPUS) {
        kpi += 10;
    } else if (actionCode == BUILD_GO8) {
        kpi += 10;
    } else if (actionCode == OBTAIN_ARC) {
        kpi += 2;
        if (hasMostARCgrants(g, player)) {
            // only add KPI if the player does not already have the
            // most ARC grants
            if (g->mostARCgrants != player) {
                kpi += 10;
                // do not subtract KPI if no player had the most
                // ARC grants
                if (g->mostARCgrants != 0) {
                    g->kpi[g->mostARCgrants-1] -= 10;
                }
                g->mostARCgrants = player;
            }
        }
    } else if (actionCode == OBTAIN_PUBLICATION) {
        if (hasMostPublications(g, player)) {
            // only add KPI if the player does not already have the
            // most publications
            if (g->mostPublications != player) {
                kpi += 10;
                // do not subtract KPI if no player had the most
                // publications
                if (g->mostPublications != 0) {
                    g->kpi[g->mostPublications-1] -= 10;
                }
                g->mostPublications = player;
            }
        }
    } else if (actionCode == OBTAIN_IP_PATENT) {
        kpi += 10;
    }

    g->kpi[player-1] += kpi;
}

static void updateArc(Game g, int x, int y, int newValue) {
    assert(y < MAP_ARC_HEIGHT);
    assert(x < MAP_ARC_WIDTH);

    // TODO assert newValue is valid
    g->arcs[y][x] = newValue;
    //printf("SET ARC %d,%d TO %d\n", x, y, newValue);
}
static void updateVertex(Game g, int x, int y, int newValue) {
    assert(y < MAP_VERTEX_HEIGHT);
    assert(x < MAP_VERTEX_WIDTH);

    // TODO assert newValue is valid

    g->vertices[y][x] = newValue;
}

static int isValidVertex(int x, int y) {
    int valid = TRUE;
    if (x > 5 || x < 0 || y < 0 || y > 10) {
        valid = FALSE;
    } else if ((y == 0 || y == 10) && !(x == 2 || x == 3)) {
        valid = FALSE;
    } else if ((y == 1 || y == 9) && (x == 0 || x == 5)) {
        valid = FALSE;
    }
    return valid;
}

static int isValidARC(int x, int y) {
    int valid = TRUE;
    // (odd, odd) and (even, even) arcs don't exist
    if (y % 2 == x % 2) {
        valid = FALSE;
    } else if (y % 2 == 0 && x % 2 == 1) {
        // if y divisible by four
        // x 3 and 7 will be invalid
        // else, x 1 5 and 9 will be invalid
        if (y % 4 == 0) {
            if (x == 3 || x == 7) {
                valid = FALSE;
            }
        } else {
            if (x == 1 || x == 5 || x == 9) {
                valid = FALSE;
            }
        }
    }

    if ((x <= 1 || x >= 9) && (y < 4 || y > 16)) { // outside board
        valid = FALSE;
    } else if ((x < 4 || x > 6) && (y > 18 || y < 2)) {
        valid = FALSE;
    }

    if (x > 10 || x < 0 || y < 0 || y > 20) {
        valid = FALSE;
    }
    return valid;
}

static int isValidAction(Game g, action a) {
    int isValid = FALSE;

    int actionType = a.actionCode;
    if (actionType == PASS ||
            actionType == BUILD_CAMPUS ||
            actionType == BUILD_GO8 ||
            actionType == OBTAIN_ARC ||
            actionType == RETRAIN_STUDENTS) {
        isValid = isLegalAction(g, a);
    } else if (actionType == OBTAIN_IP_PATENT ||
            actionType == OBTAIN_PUBLICATION) {
        action spinoff = {START_SPINOFF, "", 0, 0};
        isValid = isLegalAction(g, spinoff);
    }
    return isValid;
}

static coord getVertexCoordinateFromPath(path p) {
    return getCoordinateFromPath(p, FALSE);
}

static coord getARCCoordinateFromPath(path p) {
    return getCoordinateFromPath(p, TRUE);
}

static coord getCoordinateFromPath(path p, int getArcCoord) {
    int len = (int) strlen(p);
    #ifdef DEBUG
    printf("\n\n====== NEW PATH '%s' =====\n\n", p);
    #endif
    int i = 0;
    coord result;
    result.x = 2;
    result.y = 0;
    coord prevStop;
    prevStop.x = 1;
    prevStop.y = -1;
    while (i < len && isValidVertex(result.x, result.y)) {
        #ifdef DEBUG
        printf(" %03d of %03d - currently at (%d, %d) - previously"
                "(%d, %d) ", i+1, len, result.x, result.y, prevStop.x,
                prevStop.y);
        #endif
        char step = p[i];
        int newX = result.x;
        int newY = result.y;

        if (step == 'B') {
            if (i == 0) {
                result.x = -1;
                result.y = -1;
                step = 'Z';
            } else {
                // go back
                #ifdef DEBUG
                printf("B");
                #endif
                newX = prevStop.x;
                newY = prevStop.y;
                prevStop.x = result.x;
                prevStop.y = result.y;
                result.x = newX;
                result.y = newY;
            }
        } else if (step == 'L') {
            #ifdef DEBUG
            printf("L");
            #endif
            // work out which way is left
            // and change newX and newY
            if (result.y == prevStop.y) {
                // changing the y value
                if (result.x < prevStop.x) {
                    // facing towards the y axis
                    // so we're going down - tested in tests
                    #ifdef DEBUG
                    printf("[down]");
                    #endif
                    newY += 1;
                } else {
                    // going up - tested
                    #ifdef DEBUG
                    printf("[up]");
                    #endif
                    newY -= 1;
                }
            } else if (result.x == prevStop.x) {
                // changing the x value
                if (result.y < prevStop.y) {
                    // facing towards the x axis
                    // so we're going toward the y axis
                    if (result.y % 2 == result.x % 2) {
                        #ifdef DEBUG
                        printf("[up.]");
                        #endif
                        newY -= 1;
                    } else {
                        #ifdef DEBUG
                        printf("[left.]");
                        #endif
                        newX -= 1;
                    }
                } else {
                    if ((result.y % 2 != result.x % 2) ||
                            (result.x == 5 && result.y % 2 == 0)) {
                        #ifdef DEBUG
                        printf("[down!]");
                        #endif
                        newY += 1;
                    } else {
                        #ifdef DEBUG
                        printf("[right!]");
                        #endif
                        newX += 1;
                    }
                }

            } else {
                // this should only occur on the first step.
                // DEBUG: printf("*1");
                if (i != 0) {
                    printf("ERROR: NOT ON FIRST STEP\n");
                }
                newY = 0;
                newX = 3;
            }
            // and update the variables
            prevStop.y = result.y;
            prevStop.x = result.x;
            result.y = newY;
            result.x = newX;
        } else if (step == 'R') {
            #ifdef DEBUG
            printf("R");
            #endif
            if (result.y == prevStop.y) {
                // chainging the y value
                if (result.x < prevStop.x) {
                    // facing the y axis, so going up
                    #ifdef DEBUG
                    printf("[up]");
                    #endif
                    newY -= 1;
                } else {
                    newY += 1;
                    #ifdef DEBUG
                    printf("[down]");
                    #endif
                }
            } else if (result.x == prevStop.x) {
                // changing the x value
                if (result.y < prevStop.y) {
                    // facing the x axis
                    // going away from the y axis
                    if (result.x % 2 != result.y % 2) {
                        #ifdef DEBUG
                        printf("[up!!]");
                        #endif
                        newY -= 1;
                    } else {
                        #ifdef DEBUG
                        printf("[right!!]");
                        #endif
                        newX += 1;
                    }
                } else {
                    if ((result.x % 2 == result.y % 2)) {
                        #ifdef DEBUG
                        printf("[down!!]");
                        #endif
                        newY += 1;
                    } else {
                        #ifdef DEBUG
                        printf("[left!!]");
                        #endif
                        newX -= 1;
                    }
                }
            } else {
                // only on first step!
                if (i != 0) {
                    printf("ERROR: NOT ON FIRST STEP\n");
                }
                // DEBUG: printf("*1");
                newY = 1;
                newX = 2;
            }
            // and update the variables
            prevStop.y = result.y;
            prevStop.x = result.x;
            result.y = newY;
            result.x = newX;
        } else {
            result.y = -1;
            result.x = -1;
            step = 'Z';
        }

        #ifdef DEBUG
        printf(" (%d,%d)", result.x, result.y);
        printf("\n");
        #endif
        i++;
    }
    if (!isValidVertex(result.x, result.y)) {
        result.x = -1;
        result.y = -1;
        //printf("INVALID!\n");
    } else if (getArcCoord) {
        // convert both coordinate pairs into arcs -
        // the arc returned is the last arc traversed in the path.
        result.x *= 2;
        result.y *= 2;
        prevStop.x *= 2;
        prevStop.y *= 2;
        // and now average them out
        result.x = (result.x + prevStop.x) / 2;
        result.y = (result.y + prevStop.y) / 2;
    }
    #ifdef DEBUG
    printf("ended at (%d,%d)\n", result.x, result.y);
    #endif
    return result;
}

static int ownsARCborderingVertex(Game g, coord vertex, int player) {
    int y = vertex.y * 2;
    int x = vertex.x * 2;
    // either side
    int result = FALSE;
    if (g->arcs[y][x-1] == player || g->arcs[y][x+1] == player ||
        g->arcs[y-1][x] == player || g->arcs[y+1][x] == player) {
        result = TRUE;
    }
    return result;
}

static int ownsARCadjacentTo(Game g, coord arc, int player) {
    int y = arc.y;
    int x = arc.x;
    int result = FALSE;
    if (isValidARC(x+1, y-1) && g->arcs[y-1][x+1] == player) {
        result = TRUE;
    } else if (isValidARC(x-1, y-1) && g->arcs[y-1][x-1] == player) {
        result = TRUE;
    } else if (isValidARC(x+1, y+1) && g->arcs[y+1][x+1] == player) {
        result = TRUE;
    } else if (isValidARC(x-1, y+1) && g->arcs[y+1][x-1] == player) {
        result = TRUE;
    } else if (isValidARC(x, y+2) && g->arcs[y+2][x] == player) {
        result = TRUE;
    } else if (isValidARC(x, y-2) && g->arcs[y-2][x] == player) {
        result = TRUE;
    }
    return result;
}

static int ownsVertexBorderingARC(Game g, coord arc, int player) {
    int y = arc.y;
    int x = arc.x;
    // vertices are at an (even, even) point.
    int yDiff = y % 2;
    int xDiff = x % 2;
    int result = FALSE;
    if (yDiff > 0) {
        x = x/2;
        y++;
        if (g->vertices[y/2][x] == player ||
            g->vertices[y/2][x] == player+3) {
            result = TRUE;
        }
        y -= 2;
        if (g->vertices[y/2][x] == player ||
            g->vertices[y/2][x] == player+3) {
            result = TRUE;
        }
    } else if (xDiff > 0) {
        y = y/2;
        x++;
        if (g->vertices[y][x/2] == player ||
            g->vertices[y][x/2] == player+3) {
            result = TRUE;
        }
        x -= 2;
        if (g->vertices[y][x/2] == player ||
            g->vertices[y][x/2] == player+3) {
            result = TRUE;
        }
    }
    return result;
}

static int isCampusAdjacent(Game g, coord vertex) {
    int x = vertex.x;
    int y = vertex.y;
    int result = FALSE;
    if (y > 0 && y < MAP_VERTEX_HEIGHT) {
        if (g->vertices[y-1][x] != VACANT_VERTEX ||
                g->vertices[y+1][x] != VACANT_VERTEX) {
            result = TRUE;
        }
    }

    if (isValidARC(x*2+1, y*2)) {
        if (x < MAP_VERTEX_WIDTH) {
            if (g->vertices[y][x+1] != VACANT_VERTEX) {
                result = TRUE;
            }
        }
    } else {
        if (x > 0) {
            if (g->vertices[y][x-1] != VACANT_VERTEX) {
                result = TRUE;
             }
        }
    }
    return result;
}

static void updateExchangeRate(Game g, coord vertex, int player) {
    // special ones are:
    // 2,1 1,1 - MTV
    // 3,1 4,1 - MMONEY
    // 1,8 1,9 - BPS
    // 4,8 4,9 - MJ
    // 5,5 5,6 - BQN
    if (vertex.y == 1 && (vertex.x == 1 || vertex.x == 2)) {
        g->exchangeRates[player-1][STUDENT_MTV]--;
    } else if (vertex.y == 1 && (vertex.x == 3 || vertex.x == 4)) {
        g->exchangeRates[player-1][STUDENT_MMONEY]--;
    } else if (vertex.x == 1 && (vertex.y == 8 || vertex.y == 9)) {
        g->exchangeRates[player-1][STUDENT_BPS]--;
    } else if (vertex.x == 4 && (vertex.y == 8 || vertex.y == 9)) {
        g->exchangeRates[player-1][STUDENT_MJ]--;
    } else if (vertex.x == 5 && (vertex.y == 5 || vertex.y == 6)) {
        g->exchangeRates[player-1][STUDENT_BQN]--;
    }
}

// we did it guys!
// vim: sts=4 et cc=72
