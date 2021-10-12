#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <poll.h>
#include "pdu.h"
#include "datatypes/list.h"


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

struct NetNode {
    int UDPsocketA;
    int TCPSocketB;
    int TCPSocketC;
    int TCPSocketD;

    struct sockaddr_in UDPAddressA;
    struct sockaddr_in TCPAddressB;
    struct sockaddr_in TCPAddressC;
    struct sockaddr_in TCPAddressD;
};


typedef eSystemState(*const afEventHandler[lastState][lastEvent])(void);

typedef eSystemState(*pfEventHandler)(void); //NEEDED?


eSystemState gotoStateQ1(struct NetNode *netNode, char **argv);

eSystemState gotoStateQ2(void);

eSystemState gotoStateQ3(void);

eSystemState gotoStateQ4(void);

eSystemState gotoStateQ5(void);

eSystemState gotoStateQ6(void);

eSystemState gotoStateQ7(void);

eSystemState gotoStateQ8(void);

eSystemState gotoStateQ9(void);

eSystemState gotoStateQ10(void);

eSystemState gotoStateQ11(void);

eSystemState gotoStateQ12(void);

eSystemState gotoStateQ13(void);

eSystemState gotoStateQ14(void);

eSystemState gotoStateQ15(void);

eSystemState gotoStateQ16(void);

eSystemState gotoStateQ17(void);

eSystemState gotoStateQ18(void);

eSystemState exitState(void);

#endif