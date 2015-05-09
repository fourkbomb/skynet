/*
 *  Mr Pass.  Brain the size of a planet!
 *
 *  Proundly Created by Richard Buckland
 *  Share Freely Creative Commons SA-BY-NC 3.0. 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Game.h"
#include "mechanicalTurk.h"

action decideAction (Game g) {
    int player = getWhoseTurn(g);
    action nextAction;
    nextAction.actionCode = PASS;
    if (getStudents(g, player, STUDENT_MJ) > 0 && getStudents(g, player, STUDENT_MTV) > 0 && getStudents(g, player, STUDENT_MMONEY) > 0) {
        nextAction.actionCode = START_SPINOFF;
    }

    return nextAction;
}
