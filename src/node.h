#ifndef NODE_H
#define NODE_H

#define UDP_SOCKET_A 0  // Tracker to and from
#define TCP_SOCKET_B 1  // To succ.
#define TCP_SOCKET_C 2  // Accept new TCP 
#define TCP_SOCKET_D 3  // To pred.
#define UDP_SOCKET_A2 4 // Agent messages
#define NO_SOCKETS 5

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <poll.h>

#include "pdu.h"
#include "datatypes/list.h"
#include "datatypes/hash.h"

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
    eventInsert, //Q6-Q9
    eventLookup, //Q6-Q9
    eventRemove, //Q6-Q9
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
    eventTimeout,
    lastEvent
} eSystemEvent;

typedef struct Range {
    int min;
    int max;
} Range;

struct NetNode {
    struct pollfd fds[NO_SOCKETS];
    struct sockaddr_in fdsAddr[NO_SOCKETS];
    List *entries;
    Range nodeRange;
    unsigned char *pduMessage;
    // void *pdu_Message;
};

typedef eSystemState(*const afEventHandler[lastState][lastEvent])(struct NetNode *netNode);

typedef eSystemState(*pfEventHandler)(struct NetNode *netNode); //NEEDED?

eSystemState gotoStateQ1(struct NetNode *netNode, char **argv);

eSystemState gotoStateQ2(struct NetNode *netNode);

eSystemState gotoStateQ3(struct NetNode *netNode);

eSystemState gotoStateQ4(struct NetNode *netNode);

eSystemState gotoStateQ5(struct NetNode *netNode);

eSystemState gotoStateQ6(struct NetNode *netNode);

eSystemState gotoStateQ7(struct NetNode *netNode);

eSystemState gotoStateQ8(struct NetNode *netNode);

eSystemState gotoStateQ9(struct NetNode *netNode);

eSystemState gotoStateQ10(struct NetNode *netNode);

eSystemState gotoStateQ11(struct NetNode *netNode);

eSystemState gotoStateQ12(struct NetNode *netNode);

eSystemState gotoStateQ13(struct NetNode *netNode);

eSystemState gotoStateQ14(struct NetNode *netNode);

eSystemState gotoStateQ15(struct NetNode *netNode);

eSystemState gotoStateQ16(struct NetNode *netNode);

eSystemState gotoStateQ17(struct NetNode *netNode);

eSystemState gotoStateQ18(struct NetNode *netNode);

eSystemState exitState(struct NetNode *netNode);

#endif