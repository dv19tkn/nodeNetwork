#include <stdio.h>

typedef enum {
    q1,
    q2,
    q3,
    q4,
    q5,
    q6,
    q7,
    q8,
    q9,
    q10,
    q11,
    q12,
    q13,
    q14,
    q15,
    q16,
    q17,
    q18,
    lastState
} eSystemState;

typedef enum {
    eventStunResponse, //Q2->Q3
    eventNodeResponse, //Q3->Q7
    eventNodeResponseEmpty, //Q3->Q4
    eventJoinResponse, //Q7->Q8
    eventInsertLookupRemove, //Q6-Q9
    eventShutDown, //Q6->Q10
    eventJoin, //Q6->Q12
    eventNewRange, //Q6->Q15
    eventLeaving, //Q6->Q16
    eventCloseConnection, //Q6->Q17
    eventNewRangeResponse, //Q11-Q18
    eventConnected, //Q10-Q11
    eventNotConnected, //Q12-Q5
    eventMaxNode, //Q12-Q13
    eventNotMaxNode, //Q12-14
    eventDone, //Q*->Q*
    lastEvent
} eSystemEvent;


typedef eSystemState(*const afEventHandler[lastState][lastEvent])(void);

typedef eSystemState(*pfEventHandler)(void); //NEEDED?


eSystemState gotoStateQ1(void) {
    return q1;
}

eSystemState gotoStateQ2(void) {
    return q1;
}

eSystemState gotoStateQ3(void) {
    return q1;
}

eSystemState gotoStateQ4(void) {
    return q1;
}

eSystemState gotoStateQ5(void) {
    return q1;
}

eSystemState gotoStateQ6(void) {
    return q1;
}

eSystemState gotoStateQ7(void) {
    return q1;
}

eSystemState gotoStateQ8(void) {
    return q1;
}

eSystemState gotoStateQ9(void) {
    return q1;
}

eSystemState gotoStateQ10(void) {
    return q1;
}

eSystemState gotoStateQ11(void) {
    return q1;
}

eSystemState gotoStateQ12(void) {
    return q1;
}

eSystemState gotoStateQ13(void) {
    return q1;
}

eSystemState gotoStateQ14(void) {
    return q1;
}

eSystemState gotoStateQ15(void) {
    return q1;
}

eSystemState gotoStateQ16(void) {
    return q1;
}

eSystemState gotoStateQ17(void) {
    return q1;
}

eSystemState gotoStateQ18(void) {
    return q1;
}


static afEventHandler stateMachine = {
    [q1] = { [eventDone] = gotoStateQ2 }, //Q2
    [q2] = { [eventStunResponse] = gotoStateQ3}, //Q3 
    [q3] = { [eventNodeResponse] = }, 
    [q3] = { [eventNodeResponseEmpty] = }, 
    [q4] = { [eventDone] = }, //Q6 
    [q6] = { [eventInsertLookupRemove] = }, //Q9 
    [q6] = { [eventShutDown] = }, //Q10
    [q6] = { [eventJoin] = }, //Q12 
    [q6] = { [eventNewRange] = }, //Q15
    [q6] = { [eventLeaving] = }, //Q16
    [q6] = { [eventCloseConnection] = }, //Q17 
    [q9] = { [eventDone] = }, //Q6 
    [q10] = { [eventConnected] = }, //Q11
    [q10] = { [eventNotConnected] = },  //exit
    [q11] = { [eventNewRangeResponse] = }, //Q18 
    [q12] = { [eventNotConnected] = }, //Q5
    [q12] = { [eventMaxNode] = }, //Q13
    [q12] = { [eventNotMaxNode] = }, //Q14
    [q18] = { [eventDone] = }, //exit
    [q5] = { [eventDone] = }, //Q6
    [q13] = { [eventDone] = }, //Q6
    [q14] = { [eventDone] = }, //Q6
}

int main(int argc, char *argv[]) {


    return 0;
}