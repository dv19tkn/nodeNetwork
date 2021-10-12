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

static void exit_on_error(const char *title) {
	perror(title);
	exit(1);
}

static void exit_on_error_custom(const char *title, const char *detail) {
	fprintf(stderr, "%s:%s\n", title, detail);
	exit(1);
}

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


eSystemState gotoStateQ1(struct NetNode *netNode, char **argv) {
    char *eCheckString;
    fprintf(stderr, "entering Q1\n");
    
    //Read arguments to trackerAddress
    // memset(netNode->UDPAddressA, 0, sizeof(*netNode->UDPAddressA));
    // fprintf(stderr,"memset done\n");
    socklen_t UDPAddressLen = sizeof(netNode->UDPAddressA);
    netNode->UDPAddressA.sin_family = AF_INET;
    netNode->UDPAddressA.sin_port = htons(strtol(argv[2], NULL, 10));
    if (inet_aton(argv[1], &netNode->UDPAddressA.sin_addr) == 0) {
        exit_on_error_custom("inet_aton", argv[1]);
    }
    fprintf(stderr, "adress set\n");

    //get the set address in host byte order and print to verify
    eCheckString = inet_ntoa(netNode->UDPAddressA.sin_addr);
    if (eCheckString == NULL) {
        exit_on_error("inet_ntoa");
    }
    fprintf(stderr, "tracker address created: %s:%d\n", eCheckString, ntohs(netNode->UDPAddressA.sin_port));


    //Set up udp socket to tracker
    netNode->UDPsocketA = socket(AF_INET, SOCK_DGRAM, 0); 
    if (netNode->UDPsocketA == -1) {
        exit_on_error("socket udp tracker");
    }
    fprintf(stderr, "socket created\n");

    //Send STUN_LOOKUP
    struct STUN_LOOKUP_PDU lookupMessage = {STUN_LOOKUP};
    socklen_t lookupSize = sizeof(lookupMessage);

    if (sendto(netNode->UDPsocketA, &lookupMessage, lookupSize, 0, (struct sockaddr *) &netNode->UDPAddressA, UDPAddressLen) == -1) {
        exit_on_error("send stun lookup error");
    }
    fprintf(stderr, "stun lookup sent: %d\n", lookupMessage.type);

    return q1;
}

eSystemState gotoStateQ2(void) {
    // //Set up udp socket to tracker
    // UDPSocketA = socket(AF_INET, SOCK_DGRAM, 0); 
    // if (UDPSocketA == -1) {
    //     exit_on_error("socket udp tracker");
    // }
    // fprintf(stderr, "socket created\n");

    // //Send STUN_LOOKUP
    // struct STUN_LOOKUP_PDU lookupMessage = {STUN_LOOKUP};
    // socklen_t lookupSize = sizeof(lookupMessage);

    // if (sendto(UDPSocketA, &lookupMessage, lookupSize, 0, (struct sockaddr *) &trackerAddress, trackerAddressLen) == -1) {
    //     exit_on_error("send stun lookup error");
    // }
    // fprintf(stderr, "[Q2] stun lookup sent: %d\n", lookupMessage.type);

    return q2;
}

eSystemState gotoStateQ3(void) {
    // //Init self address
    // struct NET_GET_NODE_PDU getNodeMessage = {NET_GET_NODE};
    // socklen_t getNodeSize = sizeof(getNodeMessage);

    // if (sendto(UDPSocketA, &getNodeMessage, getNodeSize, 0, (struct sockaddr *) &trackerAddress, trackerAddressLen) == -1) {
    //     exit_on_error("send get node error");
    // }
    // fprintf(stderr, "[Q3] get node sent\n");

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


static eSystemEvent readEvent(struct NetNode *netnode) {
    struct pollfd fds[5];
    int timeoutMs = 2000;
    int returnValue;

    fds[0].fd = netnode->UDPsocketA;
    fds[0].events = POLLIN;
    fds[1].fd = netnode->TCPSocketB;
    fds[1].events = POLLIN;
    fds[2].fd = netnode->TCPSocketC;
    fds[2].events = POLLIN;
    fds[3].fd = netnode->TCPSocketD;
    fds[3].events = POLLIN;
    fds[4].fd = netnode->UDPsocketA;
    fds[4].events = POLLIN;

    returnValue = poll(fds, 5, timeoutMs);
    if (returnValue > 0) {
        for (int i = 0; i < 5; i++) {
            if (fds[i].revents & POLLIN) {
                fprintf(stderr, "1. data received on %d , return value is: %d\n", i, returnValue);
            } 
            if (fds[i].revents & POLLRDNORM) {
                fprintf(stderr, "2. data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
            if (fds[i].revents & POLLRDBAND) {
                fprintf(stderr, "3. data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
            if (fds[i].revents & POLLPRI) {
                fprintf(stderr, "4. data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
            if (fds[i].revents & POLLOUT) {
                fprintf(stderr, "5data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
            if (fds[i].revents & POLLWRNORM) {
                fprintf(stderr, "6data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
            if (fds[i].revents & POLLWRBAND) {
                fprintf(stderr, "7data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
            if (fds[i].revents & POLLERR) {
                fprintf(stderr, "8data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
            if (fds[i].revents & POLLHUP) {
                fprintf(stderr, "9data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
            if (fds[i].revents & POLLNVAL) {
                fprintf(stderr, "10data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            } 
        }
    }

    return lastEvent;
}


int main(int argc, char **argv) {
    //Handle input
    if (argc != 3) {
        exit_on_error_custom("Usage: ", " node <Tracker Address> <Tracker Port>");
    }
    fprintf(stderr, "[Q1] starting node\n");

    
    // char *eCheckString; //used to error check ip addresses 

    struct NetNode netNode = {};
    memset(&netNode, 0, sizeof(netNode));
    fprintf(stderr, "after memset: %d \n", netNode.UDPsocketA);

    eSystemState currentState = gotoStateQ1(&netNode, argv);
    eSystemEvent newEvent;

    // while (true) {
        fprintf(stderr, "current state is: %d \n", currentState + 1);
        newEvent = readEvent(&netNode);



    // }

    // eSystemEvent nextEvent = lastEvent;
    // int UDPSocketA;
    // int TCPSocketB;
    // int TCPSocketC;
    // int TCPSocketD;
    // // int UDPSocketA2;
    

    // // Read arguments to trackerAddress
    // struct sockaddr_in trackerAddress;
    // memset(&trackerAddress, 0, sizeof(trackerAddress));
    // socklen_t trackerAddressLen = sizeof(trackerAddress);
    // trackerAddress.sin_family = AF_INET;
    // trackerAddress.sin_port = htons(strtol(argv[2], NULL, 10));
    // if (inet_aton(argv[1], &trackerAddress.sin_addr) == 0) {
    //     exit_on_error_custom("inet_aton", argv[1]);
    // }

    // //get the set address in host byte order and print to verify
    // eCheckString = inet_ntoa(trackerAddress.sin_addr);
    // if (eCheckString == NULL) {
    //     exit_on_error("inet_ntoa");
    // }
    // fprintf(stderr, "tracker address created: %s:%d\n", eCheckString, ntohs(trackerAddress.sin_port));


    // //Set up udp socket to tracker
    // UDPSocketA = socket(AF_INET, SOCK_DGRAM, 0); 
    // if (UDPSocketA == -1) {
    //     exit_on_error("socket udp tracker");
    // }
    // fprintf(stderr, "socket created\n");

    // if (bind(UDPSocketA, (struct sockaddr *) &trackerAddress, trackerAddressLen) == -1) {
    //     exit_on_error("bind error");
    // }
    // fprintf(stderr, "bind done\n");

    // //Send STUN_LOOKUP
    // struct STUN_LOOKUP_PDU lookupMessage = {STUN_LOOKUP};
    // socklen_t lookupSize = sizeof(lookupMessage);

    // if (sendto(UDPSocketA, &lookupMessage, lookupSize, 0, (struct sockaddr *) &trackerAddress, trackerAddressLen) == -1) {
    //     exit_on_error("send stun lookup error");
    // }
    // fprintf(stderr, "[Q2] stun lookup sent: %d\n", lookupMessage.type);

    // //Receive STUN_RESPONS
    // struct STUN_RESPONSE_PDU responseMessage;
    // socklen_t responseSize = sizeof(responseMessage);

    // if (recvfrom(UDPSocketA, &responseMessage, responseSize, 0, (struct sockaddr *) &trackerAddress, &responseSize) == -1) {
    //     exit_on_error("recv stun response error");
    // }
    // fprintf(stderr, "recieved type: %d\n", responseMessage.type);

    // struct sockaddr_in receivedAddress;
    // memset(&receivedAddress, 0, sizeof(receivedAddress));
    // receivedAddress.sin_family = AF_INET;
    // receivedAddress.sin_addr.s_addr = responseMessage.address;
    // eCheckString = inet_ntoa(receivedAddress.sin_addr);
    // if (eCheckString == NULL) {
    //     exit_on_error("inet_ntoa");
    // }
    // fprintf(stderr, "[Q3] received address: %s\n", eCheckString);


    // //Init self address
    // struct NET_GET_NODE_PDU getNodeMessage = {NET_GET_NODE};
    // socklen_t getNodeSize = sizeof(getNodeMessage);

    // if (sendto(UDPSocketA, &getNodeMessage, getNodeSize, 0, (struct sockaddr *) &trackerAddress, trackerAddressLen) == -1) {
    //     exit_on_error("send get node error");
    // }
    // fprintf(stderr, "get node sent\n");

    // struct NET_GET_NODE_RESPONSE_PDU getNodeResponse;
    // socklen_t getNodeResponseSize = sizeof(getNodeResponse);

    // if (recvfrom(UDPSocketA, &getNodeResponse, getNodeResponseSize, 0, (struct sockaddr *) &trackerAddressLen, &getNodeResponseSize) == -1) {
    //     exit_on_error("recv get node response error");
    // }
    // fprintf(stderr, "received type: %d\n", getNodeResponse.type);

    // struct sockaddr_in preNodeAddress;
    // memset(&preNodeAddress, 0, sizeof(preNodeAddress));
    // preNodeAddress.sin_family = AF_INET;
    // preNodeAddress.sin_port = getNodeResponse.port;
    // preNodeAddress.sin_addr.s_addr = getNodeResponse.address;

    // if (preNodeAddress.sin_port == 0 && preNodeAddress.sin_addr.s_addr == 0) {
    //     fprintf(stderr, "[Q4] got empty reponse\n");
    //     //Init table
    //     // List *list = list_create();

    // } else {
    //     fprintf(stderr, "[Q7] received address: %s:%d\n", inet_ntoa(preNodeAddress.sin_addr), ntohs(preNodeAddress.sin_port));
    // }




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


    // close(UDPSocketA);
    close(netNode.UDPsocketA);
    // close(socketTcpC);
    fprintf(stderr, "exiting node\n");
    return 0;
}