#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "pdu.h"

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
    return q2;
}

eSystemState gotoStateQ3(void) {
    return q3;
}

eSystemState gotoStateQ4(void) {
    return q4;
}

eSystemState gotoStateQ5(void) {
    return q5;
}

eSystemState gotoStateQ6(void) {
    return q6;
}

eSystemState gotoStateQ7(void) {
    return q7;
}

eSystemState gotoStateQ8(void) {
    return q8;
}

eSystemState gotoStateQ9(void) {
    return q9;
}

eSystemState gotoStateQ10(void) {
    return q10;
}

eSystemState gotoStateQ11(void) {
    return q11;
}

eSystemState gotoStateQ12(void) {
    return q12;
}

eSystemState gotoStateQ13(void) {
    return q13;
}

eSystemState gotoStateQ14(void) {
    return q14;
}

eSystemState gotoStateQ15(void) {
    return q15;
}

eSystemState gotoStateQ16(void) {
    return q16;
}

eSystemState gotoStateQ17(void) {
    return q17;
}

eSystemState gotoStateQ18(void) {
    return q18;
}

eSystemState exitState(void) {
    return lastState;
}


static afEventHandler stateMachine = {
    [q1] = { [eventDone] = gotoStateQ2 }, //Q2
    [q2] = { [eventStunResponse] = gotoStateQ3}, //Q3 
    [q3] = { [eventNodeResponse] = gotoStateQ7}, 
    [q3] = { [eventNodeResponseEmpty] = gotoStateQ4}, 
    [q4] = { [eventDone] = gotoStateQ6}, //Q6 
    [q6] = { [eventInsertLookupRemove] = gotoStateQ9}, //Q9 
    [q6] = { [eventShutDown] = gotoStateQ10}, //Q10
    [q6] = { [eventJoin] = gotoStateQ12}, //Q12 
    [q6] = { [eventNewRange] = gotoStateQ15}, //Q15
    [q6] = { [eventLeaving] = gotoStateQ16}, //Q16
    [q6] = { [eventCloseConnection] = gotoStateQ17}, //Q17 
    [q9] = { [eventDone] = gotoStateQ6}, //Q6 
    [q10] = { [eventConnected] = gotoStateQ11}, //Q11
    [q10] = { [eventNotConnected] = exitState},  //exit
    [q11] = { [eventNewRangeResponse] = gotoStateQ18}, //Q18 
    [q12] = { [eventNotConnected] = gotoStateQ5}, //Q5
    [q12] = { [eventMaxNode] = gotoStateQ13}, //Q13
    [q12] = { [eventNotMaxNode] = gotoStateQ14}, //Q14
    [q18] = { [eventDone] = exitState}, //exit
    [q5] = { [eventDone] = gotoStateQ6}, //Q6
    [q13] = { [eventDone] = gotoStateQ6}, //Q6
    [q14] = { [eventDone] = gotoStateQ6} //Q6
};

static void exit_on_error(const char *title) {
	perror(title);
	exit(1);
}

static void exit_on_error_custom(const char *title, const char *detail) {
	fprintf(stderr, "%s:%s\n", title, detail);
	exit(1);
}

int main(int argc, char *argv[]) {
    // char eCheckString[INET_ADDRSTRLEN]; //used to error check ip addresses 
    char *eCheckString;
    // eSystemState currentState = q1;
    // eSystemEvent nextEvent = lastEvent;

    fprintf(stderr, "starting node\n");


    //Handle input
    if (argc != 3) {
        exit_on_error_custom("Usage: ", " node <Tracker Address> <Tracker Port>");
    }


    //Read arguments to trackerAddress
    struct sockaddr_in trackerAddress;
    memset(&trackerAddress, 0, sizeof(trackerAddress));
    socklen_t trackerAddressLen = sizeof(trackerAddress);

    trackerAddress.sin_family = AF_INET;
    trackerAddress.sin_port = htons(strtol(argv[2], NULL, 10));
    if (inet_aton(argv[1], &trackerAddress.sin_addr) == 0) {
        exit_on_error_custom("inet_aton", argv[1]);
    }

    //get the set address in host byte order and print to verify
    eCheckString = inet_ntoa(trackerAddress.sin_addr);
    if (eCheckString == NULL) {
        exit_on_error("inet_ntoa");
    }
    fprintf(stderr, "tracker address created: %s:%d\n", eCheckString, ntohs(trackerAddress.sin_port));


    //Set up udp socket to tracker
    int socketUdpTracker = socket(AF_INET, SOCK_DGRAM, 0); //IPPROTO_UDP??
    if (socketUdpTracker == -1) {
        exit_on_error("socket udp tracker");
    }
    fprintf(stderr, "socket created\n");

    //Send STUN_LOOKUP
    struct STUN_LOOKUP_PDU lookupMessage = {STUN_LOOKUP};
    socklen_t lookupSize = sizeof(lookupMessage);

    if (sendto(socketUdpTracker, &lookupMessage, lookupSize, 0, (struct sockaddr *) &trackerAddress, trackerAddressLen) == -1) {
        exit_on_error("send stun lookup error");
    }
    fprintf(stderr, "stun lookup sent: %d\n", lookupMessage.type);

    //Receive STUN_RESPONS
    struct STUN_RESPONSE_PDU responseMessage;
    socklen_t responseSize = sizeof(responseMessage);

    if (recvfrom(socketUdpTracker, &responseMessage, responseSize, 0, (struct sockaddr *) &trackerAddress, &responseSize) == -1) {
        exit_on_error("recv stun response error");
    }
    fprintf(stderr, "recieved type: %d\n", responseMessage.type);

    struct sockaddr_in receivedAddress;
    memset(&receivedAddress, 0, sizeof(receivedAddress));
    receivedAddress.sin_family = AF_INET;
    receivedAddress.sin_addr.s_addr = responseMessage.address;
    eCheckString = inet_ntoa(receivedAddress.sin_addr);
    if (eCheckString == NULL) {
        exit_on_error("inet_ntoa");
    }
    fprintf(stderr, "received address: %s\n", eCheckString);


    //Init self address
    struct NET_GET_NODE_PDU getNodeMessage = {NET_GET_NODE};
    socklen_t getNodeSize = sizeof(getNodeMessage);

    if (sendto(socketUdpTracker, &getNodeMessage, getNodeSize, 0, (struct sockaddr *) &trackerAddress, trackerAddressLen) == -1) {
        exit_on_error("send get node error");
    }
    fprintf(stderr, "get node sent\n");

    struct NET_GET_NODE_RESPONSE_PDU getNodeResponse;
    socklen_t getNodeResponseSize = sizeof(getNodeResponse);

    if (recvfrom(socketUdpTracker, &getNodeResponse, getNodeResponseSize, 0, (struct sockaddr *) &trackerAddressLen, &getNodeResponseSize) == -1) {
        exit_on_error("recv get node response error");
    }
    fprintf(stderr, "received type: %d\n", getNodeResponse.type);

    struct sockaddr_in preNodeAddress;
    memset(&preNodeAddress, 0, sizeof(preNodeAddress));
    preNodeAddress.sin_family = AF_INET;
    preNodeAddress.sin_port = getNodeResponse.port;
    preNodeAddress.sin_addr.s_addr = getNodeResponse.address;

    if (preNodeAddress.sin_port == 0 && preNodeAddress.sin_addr.s_addr == 0) {
        fprintf(stderr, "got empty reponse\n");
    } else {
        fprintf(stderr, "received address: %s:%d\n", inet_ntoa(preNodeAddress.sin_addr), ntohs(preNodeAddress.sin_port));
    }

    //Init TCP C (GÖRS SENARE)
    // int socketTcpC = socket(AF_INET, SOCK_STREAM, 0);
    // if (socketTcpC == -1) {
    //     exit_on_error("socket");
    // }
    // fprintf(stderr, "socket created\n");

    // struct sockaddr_in socketCAddress;
    // memset(&socketCAddress, 0, sizeof(socketCAddress));
    // socketCAddress.sin_family = AF_INET;
    // socketCAddress.sin_port = 0;
    // socketCAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // socklen_t socketTcpCLen = sizeof(socketCAddress);

    // if (bind(socketTcpC, (struct sockaddr *) &socketCAddress, socketTcpCLen) == -1) {
    //     exit_on_error("bind");
    // }
    // fprintf(stderr, "bind done\n");


    close(socketUdpTracker);
    // close(socketTcpC);
    fprintf(stderr, "exiting node\n");
    return 0;
}