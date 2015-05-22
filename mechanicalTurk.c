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
static path *_findNextVacantARC(Game g, path *startingPath, 
	char nextStep, int depth);
static path *findNextVacantARC(Game g, path *startingPath);

action decideAction (Game g) {
    int player = getWhoseTurn(g);
    action nextAction = {PASS, "", 0, 0};
    if (getStudents(g, player, STUDENT_BPS) > 0
    	&& getStudents(g, player, STUDENT_BQN) > 0) {
    	nextAction.actionCode = OBTAIN_ARC;
    	path dest = {0};
    	if (player == UNI_A) {
    		strncpy(dest, UNI_A_CAMPUS_2, strlen(UNI_A_CAMPUS_2));
    	} else if (player == UNI_B) {
    		strncpy(dest, UNI_B_CAMPUS_2, strlen(UNI_B_CAMPUS_2));
    	} else if (player == UNI_C) {
			strncpy(dest, UNI_C_CAMPUS_2, strlen(UNI_C_CAMPUS_2));
    	}
    	path orig = {0};
    	strncpy(orig, dest, strlen(dest));
    	while (getARC(g, dest) != VACANT_ARC && strlen(dest) > 0) {
    		path new = {0};
    		strncpy(new, dest, strlen(dest)-1);
    		strncpy(dest, new, PATH_LIMIT-1);
    	}
    	if (strlen(dest) == 0) {
    		// cry
    		// go left or right depending on what's legal
    		path *ptr = findNextVacantARC(g, &orig);
    		if (*ptr[0] == 'x') {
    			// give up!
    			#ifdef AI_DEBUG
    			printf("Couldn't find any usable ARCs!\n");
    			#endif
    			nextAction.actionCode = PASS;
    		}
    		strncpy(dest, *ptr, strlen(*ptr));
    		free(ptr);

    	}
    	strncpy(nextAction.destination, dest, PATH_LIMIT-1);
    } else if (getStudents(g, player, STUDENT_BPS) < 1) {
    	// convert from * to BPS
    	nextAction.actionCode = RETRAIN_STUDENTS;
    	nextAction.disciplineTo = STUDENT_BPS;
    	if (getStudents(g, player, STUDENT_MJ) >=
    		getExchangeRate(g, player, STUDENT_MJ, STUDENT_BPS)) {
    		nextAction.disciplineFrom = STUDENT_MJ;
    	} else if (getStudents(g, player, STUDENT_MTV) >=
    		getExchangeRate(g, player, STUDENT_MTV, STUDENT_BPS)) {
    		nextAction.disciplineFrom = STUDENT_MTV;
    	} else if (getStudents(g, player, STUDENT_MMONEY) >=
    		getExchangeRate(g, player, STUDENT_MMONEY, STUDENT_BPS)) {
    		nextAction.disciplineFrom = STUDENT_MMONEY;
    	} else if (getStudents(g, player, STUDENT_BQN) >=
    		getExchangeRate(g, player, STUDENT_BQN, STUDENT_BPS)+1) {
    		// we want to have 1 bps left over so that
    		// we can build an ARC.
    		nextAction.disciplineFrom = STUDENT_BQN;
    	} else {
    		nextAction.actionCode = PASS;
    	}
    } else if (getStudents(g, player, STUDENT_BQN) < 1) {
    	nextAction.actionCode = RETRAIN_STUDENTS;
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

static path *_findNextVacantARC(Game g, path *startingPath, 
	char nextStep, int depth) {
	path temp = {0};
	strncat(&(temp[0]), *startingPath, strlen(*startingPath));
	strncpy(&temp[strlen(*startingPath)], &nextStep, 1);
	path *result;
	#ifdef AI_DEBUG
	printf("%s => %s %d\n", *startingPath, temp, depth);
	#endif
	if (depth < 25) {
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
			action checkIsOk = {OBTAIN_ARC, "", 0, 0};
			#ifdef AI_DEBUG
			printf("found a vacant arc! ");
			#endif
			strncpy(checkIsOk.destination, temp, strlen(temp));
			if (isLegalAction(g, checkIsOk)) {
				#ifdef AI_DEBUG
				printf("it's legal! hooray!\n");
				#endif
				result = (path*)strndup(temp, PATH_LIMIT);
			} else {
				#ifdef AI_DEBUG
				printf("it's illegal :(\n");
				#endif
				strncpy(temp, "x\0", 2);
				result = (path*)strndup(temp, PATH_LIMIT);
			}
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
