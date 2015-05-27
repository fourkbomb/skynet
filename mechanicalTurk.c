/*
 *  Mr Pass.  Brain the size of a planet!
 *
 *  Proundly Created by Richard Buckland
 *  Share Freely Creative Commons SA-BY-NC 3.0. 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Game.h"
#include "mechanicalTurk.h"

#define DEFAULT_DISCIPLINES {STUDENT_BQN, STUDENT_MMONEY, STUDENT_MJ, \
        STUDENT_MMONEY, STUDENT_MJ, STUDENT_BPS, STUDENT_MTV, \
        STUDENT_MTV, STUDENT_BPS, STUDENT_MTV, STUDENT_BQN, \
        STUDENT_MJ, STUDENT_BQN, STUDENT_THD, STUDENT_MJ, \
        STUDENT_MMONEY, STUDENT_MTV, STUDENT_BQN, STUDENT_BPS}
#define DEFAULT_DICE {9, 10, 8, 12, 6, 5, 3, 11, 3, 11, 4, 6, 4, 7, \
        9, 2, 8, 10, 5}

#define UNI_A_CAMPUS_1 ""
#define UNI_A_CAMPUS_2 "RLRLRLRLLRR"
#define UNI_B_CAMPUS_1 "RRLRL"
#define UNI_B_CAMPUS_2 "LRLRLRRLRL"
#define UNI_C_CAMPUS_1 "LRLRL"
#define UNI_C_CAMPUS_2 "RRLRLLRLRL"

// tests
/*int main(void) {
	int disciplines[] = DEFAULT_DISCIPLINES;
    int diceValues[] = DEFAULT_DICE;
    Game g = newGame(disciplines, diceValues);

	throwDice(g, 1);
	assert(getCampus(g, UNI_A_CAMPUS_1) == UNI_A);
	assert(getCampus(g, UNI_A_CAMPUS_2) == UNI_A);
	assert(getCampus(g, UNI_B_CAMPUS_1) == UNI_B);
	assert(getCampus(g, UNI_B_CAMPUS_2) == UNI_B);
	assert(getCampus(g, UNI_C_CAMPUS_1) == UNI_C);
	assert(getCampus(g, UNI_C_CAMPUS_2) == UNI_C);
	return 0;	
}*/

typedef struct _coord {
    int x;
    int y;
} coord;
static path *_findNextVacantARC(Game g, path *startingPath, 
	char nextStep, int depth);
static path *findNextVacantARC(Game g, path *startingPath);
static action tryConvertTo(Game g, int studentTo, int *minTypes);
static path *findNextVacantCampusSpot(Game g, path *startingPath);
static path *_findNextVacantCampusSpot(Game g, path *startingPath, 
    char nextStep, int depth);
static coord getVertexCoordinateFromPath(path p);
static coord getCoordinateFromPath(path p, int getArcCoord);
static coord getARCCoordinateFromPath(path p);
static int isValidVertex(int x, int y);

action decideAction (Game g) {
    action nextAction = {PASS, "", 0, 0};
    int campusesExhausted = 0;
    int arcsExhausted = 0;
    path myCampus = {0};

    int player = getWhoseTurn(g);
    int rng = rand() % 2;
    if (player == UNI_A) {
        strncpy(myCampus, rng ? UNI_A_CAMPUS_1 : UNI_A_CAMPUS_2, strlen(rng ? UNI_A_CAMPUS_1 : UNI_A_CAMPUS_2));
    } else if (player == UNI_B) {
        strncpy(myCampus, rng ? UNI_B_CAMPUS_1 : UNI_B_CAMPUS_2, strlen(rng ? UNI_B_CAMPUS_1 : UNI_B_CAMPUS_2));
    } else if (player == UNI_C) {
        strncpy(myCampus, rng ? UNI_C_CAMPUS_1 : UNI_C_CAMPUS_2, strlen(rng ? UNI_C_CAMPUS_1 : UNI_C_CAMPUS_2));
    }

    path *ptrCampuses = findNextVacantCampusSpot(g, &myCampus);

    path *ptrARCs = findNextVacantARC(g, &myCampus);
    if (*ptrARCs[0] == 'x') {
        printf("exhausted arcs\n");
        arcsExhausted = 1;
        if (*ptrCampuses[0] == 'x') {
            printf("exhausted campuses\n");
            campusesExhausted = 1;
        }
    }

    if (getStudents(g, player, STUDENT_MJ) > 0
        && getStudents(g, player, STUDENT_BQN) > 0
        && getStudents(g, player, STUDENT_BPS) > 0
        && getStudents(g, player, STUDENT_MTV) > 0) {
        #ifdef AI_DEBUG_CAMPUS
        printf("BUILDING A CAMPUS BECAUSE I HAVE THE RIGHT STUDENTS\n");
        #endif

        path dest = {0};
        if (*ptrCampuses[0] == 'x') {
            #ifdef AI_DEBUG_CAMPUS
            printf("Couldn't find any usable campuses!\n");
            #endif
        } else {
            strncpy(dest, *ptrCampuses, strlen(*ptrCampuses));
            nextAction.actionCode = BUILD_CAMPUS;
            strncpy(nextAction.destination, dest, PATH_LIMIT-1);

            if (!isLegalAction(g, nextAction)) {
                nextAction.actionCode = PASS;
            }
        }
    }

    free(ptrCampuses);

    if (getStudents(g, player, STUDENT_BPS) > 0
    	&& getStudents(g, player, STUDENT_BQN) > 0
        && nextAction.actionCode == PASS) {
    	nextAction.actionCode = OBTAIN_ARC;

		path *ptr = findNextVacantARC(g, &myCampus);
		if (*ptr[0] == 'x') {
			// give up!
			#ifdef AI_DEBUG
			printf("Couldn't find any usable ARCs!\n");
			#endif
			nextAction.actionCode = PASS;
		}

        path dest = {0};
		strncpy(dest, *ptr, strlen(*ptr));

		free(ptr);

    	strncpy(nextAction.destination, dest, PATH_LIMIT-1);
        if (!isLegalAction(g, nextAction)) {
            nextAction.actionCode = PASS;
        }
    }

    free(ptrARCs);

    if (getStudents(g, player, STUDENT_MJ) > 0
        && getStudents(g, player, STUDENT_MMONEY) > 0
        && getStudents(g, player, STUDENT_MTV) > 0
        && nextAction.actionCode == PASS) {
        nextAction.actionCode = START_SPINOFF;
    } else if (nextAction.actionCode == PASS) {
    	/*nextAction.actionCode = RETRAIN_STUDENTS;
    	nextAction.disciplineTo = STUDENT_BQN;
    	if (getStudents(g, player, STUDENT_MJ) >=
    		getExchangeRate(g, player, STUDENT_MJ, STUDENT_BQN)) {
    		nextAction.disciplineFrom = STUDENT_MJ;
    	} else if (getStudents(g, player, STUDENT_MTV) >=
    		getExchangeRate(g, player, STUDENT_MTV, STUDENT_BQN)) {
    		nextAction.disciplineFrom = STUDENT_MTV;
    	} else if (getStudents(g, player, STUDENT_MMONEY) >=
    		getExchangeRate(g, player, STUDENT_MMONEY, STUDENT_BQN)) {
    		nextAction.disciplineFrom = STUDENT_MMONEY;
    	} else if (getStudents(g, player, STUDENT_BPS) >=
    		getExchangeRate(g, player, STUDENT_BPS, STUDENT_BQN)+1) {
    		nextAction.disciplineFrom = STUDENT_BPS;
    	} else {
    		nextAction.actionCode = PASS;
    	}*/
        // we want to try and have at least one of each type
        action r;
        int types[] = {0, 1, 1, 1, 1, 1};
        /*if (getStudents(g, player, STUDENT_BPS) < 1) {
            r = tryConvertTo(g, STUDENT_BPS, types);
        } else if (getStudents(g, player, STUDENT_BQN) < 1) {
            r = tryConvertTo(g, STUDENT_BQN, types);
        } else if (getStudents(g, player, STUDENT_MJ) < 1) {
            r = tryConvertTo(g, STUDENT_MJ, types);
        } else if (getStudents(g, player, STUDENT_MTV) < 1) {
            r = tryConvertTo(g, STUDENT_MTV, types);
        } else if (getStudents(g, player, STUDENT_MMONEY) < 1) {
            r = tryConvertTo(g, STUDENT_MMONEY, types);
        }*/
        int smallestType = 0;
        int smallestCount = 9000;
        for (int i = STUDENT_BPS; i <= STUDENT_MMONEY; i++) {
            if (getStudents(g, player, i) < smallestCount) {
                smallestCount = getStudents(g, player, i);
                smallestType = i;
            }
        }

        if (smallestType != 0) {
            r = tryConvertTo(g, smallestType, types);
        } else {
            r.actionCode = PASS;
        }

        if (r.disciplineFrom != 0) {
            nextAction.disciplineFrom = r.disciplineFrom;
            nextAction.disciplineTo = r.disciplineTo;
            nextAction.actionCode = r.actionCode;
        }
    }
    /*
    if (getStudents(g, player, STUDENT_MJ) > 0 
    	&& getStudents(g, player, STUDENT_MTV) > 0 
    	&& getStudents(g, player, STUDENT_MMONEY) > 0) {
        nextAction.actionCode = START_SPINOFF;
    }*/

    return nextAction;
}

static action tryConvertTo(Game g, int studentTo, int *minTypes) {
    action a = {RETRAIN_STUDENTS, "", 0, 0};
    int successful = 0;
    int studentFrom = 0;
    for (int i = STUDENT_BPS; i <= STUDENT_MMONEY && successful != 1; i++) {
        if (i != studentTo) {
            if ((getStudents(g, getWhoseTurn(g), i) - getExchangeRate(g, getWhoseTurn(g), i, studentTo)) >= minTypes[i]) {
                successful = 1;
                studentFrom = i;
            }
        }
    }

    if (successful) {
        //action a = {RETRAIN_STUDENTS, "", studentFrom, studentTo};
        a.disciplineFrom = studentFrom;
        a.disciplineTo = studentTo;
        if (!isLegalAction(g, a)) {
            a.disciplineFrom = 0;
            a.disciplineTo = 0;
        }
    }

    return a;
}

static path *_findNextVacantARC(Game g, path *startingPath, 
	char nextStep, int depth) {
	path temp = {0};
	strncat(&(temp[0]), *startingPath, strlen(*startingPath));
	strncpy(&temp[strlen(*startingPath)], &nextStep, 1);

	path *result;

	#ifdef AI_DEBUG
	printf("%s => %s %d\n", *startingPath, temp, depth);
	#endif

    coord c = getARCCoordinateFromPath(temp);
    if (c.x == -1 || c.y == -1) {
        #ifdef AI_DEBUG
        printf("invalid coordinates! aborting.\n");
        #endif

        strncpy(temp, "x\0", 2);
        result = (path*)strndup(temp, PATH_LIMIT);
    } else {
    	if (depth < 20) {
    		if (getARC(g, temp) == getWhoseTurn(g)) {
    			// keep going down this path
    			#ifdef AI_DEBUG
    			printf("found an arc owned by me, following: %d\n",
    				depth+1);
    			#endif

    			result = _findNextVacantARC(g, &temp, 'R', depth+1);

    			#ifdef AI_DEBUG
    			printf("GOT A RESULT\n");
    			printf("%s\n", *result);
    			#endif

    			if (*result[0] == 'x') {
    				result = _findNextVacantARC(g, &temp, 'L', depth+1);
    			}
    		} else if (getARC(g, temp) == VACANT_ARC) {
    			#ifdef AI_DEBUG
    			printf("found a vacant arc! ");
    			#endif

    			// if (isLegalAction(g, checkIsOk)) {
    			#ifdef AI_DEBUG
    			printf("it's legal! hooray!\n");
    			#endif

    			result = (path*)strndup(temp, PATH_LIMIT);
    			/*} else {
    				#ifdef AI_DEBUG
    				printf("it's illegal :(\n");
    				#endif
    				strncpy(temp, "x\0", 2);
    				result = (path*)strndup(temp, PATH_LIMIT);
    			}*/
    		} else {
    			#ifdef AI_DEBUG
    			printf("arc is owned by someone else :(\n");
    			#endif
    			// arc is owned by another uni.
    			//result = "x\0";
    			strncpy(temp, "x\0", 2);
    			result = (path*)strndup(temp, PATH_LIMIT);
    		}
    	} else {
    		#ifdef AI_DEBUG
    		printf("I've gone too damn far\n");
    		#endif

    		strncpy(temp, "x\0", 2);
    		result = (path*)strndup(temp, PATH_LIMIT);
    	}
    }

	//printf("result: %s\n", *result);
	#ifdef AI_DEBUG
	if (depth == 0) {
		printf("Going to %s", *result);
	}
	#endif

	return result;
}

static path *findNextVacantARC(Game g, path *startingPath) {
	path* res = _findNextVacantARC(g, startingPath, 'R', 0);
	if (*res[0] == 'x') {
		free(res);
		res = _findNextVacantARC(g, startingPath, 'L', 0);
	}

	#ifdef AI_DEBUG
	printf("\n\nGoing to %s\n\n", *res);
	#endif

	return res;
}

static path *_findNextVacantCampusSpot(Game g, path *startingPath, 
    char nextStep, int depth) {
    path temp = {0};
    strncat(&(temp[0]), *startingPath, strlen(*startingPath));
    strncpy(&temp[strlen(*startingPath)], &nextStep, 1);

    int done = 0;

    #ifdef AI_DEBUG_CAMPUS
    printf("\n[campus] === depth %d ===\n\n", depth);
    //printf("[campus] %s => %s %d\n", *startingPath, temp, depth);
    #endif

    path *result;
    coord testPath = getVertexCoordinateFromPath(temp);
    if (testPath.x == -1 || testPath.y == -1) {
        #ifdef AI_DEBUG_CAMPUS
        printf("[campus] Invalid coordinates (%d,%d)\n", testPath.x, testPath.y);
        #endif

        strncpy(temp, "x\0", 2);
        result = (path*) strndup(temp, PATH_LIMIT);
    } else {
        if (depth < 15) {
            if (getCampus(g, temp) == VACANT_VERTEX) {
                // this can be illegal if there's
                // 1. no leading arc
                // 2. a campus adjacent

                #ifdef AI_DEBUG_CAMPUS
                printf("[campus] vertex is valid: owned by %d, i am %d\n", getCampus(g, temp), getWhoseTurn(g));
                #endif

                action checkIsOk = {BUILD_CAMPUS, "", 0, 0};
                strncpy(checkIsOk.destination, temp, PATH_LIMIT);

                #ifdef AI_DEBUG_CAMPUS
                printf("[campus] isLegalAction(g, {BUILD_CAMPUS, %s, 0, 0})\n", checkIsOk.destination);
                #endif

                if (isLegalAction(g, checkIsOk)) {
                    #ifdef AI_DEBUG_CAMPUS
                    printf("!@$!$#!$#!$!#$!#$ ACTION action is legal. building campus here.\n");
                    #endif

                    result = (path*)strndup(temp, PATH_LIMIT);
                    done = 1;
                } else {
                    #ifdef AI_DEBUG_CAMPUS
                    printf("action is illegal!\n");
                    #endif
                }
            }

            if (!done && getARC(g, temp) == getWhoseTurn(g)) {
                #ifdef AI_DEBUG_CAMPUS
                printf("[campus] arc is ok to follow: owned by %d, i am %d, done: %d\n", getARC(g, temp), getWhoseTurn(g), done);
                #endif

                result = _findNextVacantCampusSpot(g, &temp, 'R', depth+1);
                if (*result[0] == 'x') {
                    #ifdef AI_DEBUG_CAMPUS
                    printf("[campus] going right failed\n");
                    #endif
                    result = _findNextVacantCampusSpot(g, &temp, 'L', depth+1);
                }

                done = 1;
            } else if (done == 0) {
                #ifdef AI_DEBUG_CAMPUS
                printf("[campus] arc is not ok: owned by %d, i am %d\n", getARC(g, temp), getWhoseTurn(g));
                #endif

                strncpy(temp, "x\0", 2);
                result = (path*)strndup(temp, PATH_LIMIT);
            }
        } else {
            #ifdef AI_DEBUG_CAMPUS
            printf("[campus] too many levels of recursion!\n");
            #endif

            strncpy(temp, "x\0", 2);
            result = (path*)strndup(temp, PATH_LIMIT);
        }
    }
    /*if (depth < 25) {
        if (getCampus(g, temp) == VACANT_VERTEX) {
            action checkIsOk = {BUILD_CAMPUS, "", 0, 0};
            #ifdef AI_DEBUG_CAMPUS
            printf("found a vacant vertex! %s ", temp);
            #endif
            strncpy(checkIsOk.destination, temp, strlen(temp));
            coord dest = getVertexCoordinateFromPath(temp);
            if (isLegalAction(g, checkIsOk) && dest.x != -1 && dest.y != -1) {
                #ifdef AI_DEBUG_CAMPUS
                printf("it's legal! hooray!\n");
                #endif
                result = (path*)strndup(temp, PATH_LIMIT);
                done = 1;
            } else {
                #ifdef AI_DEBUG_CAMPUS
                printf("it's illegal: owned by %d, leading arc is owned by %d, i am %d :(\n", getCampus(g, temp), getARC(g, temp), getWhoseTurn(g));
                #endif
                done = 0;
                //strncpy(temp, "x\0", 2);
            }
        }
        if (done == 0 && getARC(g, temp) == getWhoseTurn(g)) {
            // keep going down this path
            #ifdef AI_DEBUG_CAMPUS
            printf("found an arc owned by me, following: %d\n",
                depth+1);
            #endif
            result = _findNextVacantCampusSpot(g, &temp, 'R', depth+1);
            #ifdef AI_DEBUG_CAMPUS
            printf("GOT A RESULT: ");
            printf("%s\n", *result);
            #endif
            if (*result[0] == 'x') {
                result = _findNextVacantCampusSpot(g, &temp, 'L', depth+1);
            }
        } else if (done == 0) {
            #ifdef AI_DEBUG_CAMPUS
            printf("arc is owned by someone else :( done: %d, arc: %d, me: %d\n", done, getARC(g, temp), getWhoseTurn(g));
            #endif
            // arc is owned by another uni.
            //result = "x\0";
            strncpy(temp, "x\0", 2);
            result = (path*)strndup(temp, PATH_LIMIT);
        }
    } else {
        #ifdef AI_DEBUG_CAMPUS
        printf("I've gone too damn far\n");
        #endif
        strncpy(temp, "x\0", 2);
        result = (path*)strndup(temp, PATH_LIMIT);
    }*/
    //printf("result: %s\n", *result);
 
    #ifdef AI_DEBUG_CAMPUS
    if (depth == 0) {
        printf("Going to %s\n", *result);
    }
    printf("\n=== end depth %d ===\n\n", depth);
    #endif

    return result;
}
static path *findNextVacantCampusSpot(Game g, path *startingPath) {
    path* res = _findNextVacantCampusSpot(g, startingPath, 'R', 0);
    if (*res[0] == 'x') {
        free(res);
        res = _findNextVacantCampusSpot(g, startingPath, 'L', 0);
    }

    #ifdef AI_DEBUG_CAMPUS
    printf("\n\nGoing to %s!!!\n\n", *res);
    #endif

    return res;
}
// coordinate functions

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

    coord result;
    result.x = 2;
    result.y = 0;

    coord prevStop;
    prevStop.x = 1;
    prevStop.y = -1;

    int i = 0;
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
                    #ifdef DEBUG
                    printf("[down]");
                    #endif

                    newY += 1;
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
    }

    if (getArcCoord) {
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
