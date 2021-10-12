#include "node.h"


static void check_params(int argc);
static void exit_on_error(const char *title);
static void exit_on_error_custom(const char *title, const char *detail);
static eSystemEvent readEvent(struct NetNode *netnode);
static afEventHandler stateMachine = {
    [q1] = {[eventDone] = gotoStateQ2},         //Q2
    [q2] = {[eventStunResponse] = gotoStateQ3}, //Q3
    [q3] = {[eventNodeResponse] = gotoStateQ7},
    [q3] = {[eventNodeResponseEmpty] = gotoStateQ4},
    [q4] = {[eventDone] = gotoStateQ6},               //Q6
    [q6] = {[eventInsertLookupRemove] = gotoStateQ9}, //Q9
    [q6] = {[eventShutDown] = gotoStateQ10},          //Q10
    [q6] = {[eventJoin] = gotoStateQ12},              //Q12
    [q6] = {[eventNewRange] = gotoStateQ15},          //Q15
    [q6] = {[eventLeaving] = gotoStateQ16},           //Q16
    [q6] = {[eventCloseConnection] = gotoStateQ17},   //Q17
    [q9] = {[eventDone] = gotoStateQ6},               //Q6
    [q10] = {[eventConnected] = gotoStateQ11},        //Q11
    [q10] = {[eventNotConnected] = exitState},        //exit
    [q11] = {[eventNewRangeResponse] = gotoStateQ18}, //Q18
    [q12] = {[eventNotConnected] = gotoStateQ5},      //Q5
    [q12] = {[eventMaxNode] = gotoStateQ13},          //Q13
    [q12] = {[eventNotMaxNode] = gotoStateQ14},       //Q14
    [q18] = {[eventDone] = exitState},                //exit
    [q5] = {[eventDone] = gotoStateQ6},               //Q6
    [q13] = {[eventDone] = gotoStateQ6},              //Q6
    [q14] = {[eventDone] = gotoStateQ6}               //Q6
};

int main(int argc, char **argv)
{
    check_params(argc);

    struct NetNode netNode = {};
    memset(&netNode, 0, sizeof(netNode));
    fprintf(stderr, "after memset: %d \n", netNode.UDPsocketA);

    eSystemState currentState = gotoStateQ1(&netNode, argv);
    eSystemEvent newEvent;
    fprintf(stderr, "[Q1] starting node\n");

    // while (true) {
    fprintf(stderr, "current state is: %d \n", currentState + 1);
    newEvent = readEvent(&netNode);

    // }

    // close(UDPSocketA);
    close(netNode.UDPsocketA);
    // close(socketTcpC);
    fprintf(stderr, "exiting node\n");
    return 0;
}

static void check_params(int argc)
{
    if (argc != 3)
    {
        exit_on_error_custom("Usage: ", " node <Tracker Address> <Tracker Port>");
    }
}

static void exit_on_error(const char *title)
{
    perror(title);
    exit(1);
}

static void exit_on_error_custom(const char *title, const char *detail)
{
    fprintf(stderr, "%s:%s\n", title, detail);
    exit(1);
}

static eSystemEvent readEvent(struct NetNode *netnode)
{
    struct pollfd fds[5];
    int timeoutMs = 2000;
    int returnValue;
    unsigned char buffer[1024];

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
    if (returnValue == -1)
    {
        exit_on_error("poll error");
    }

    if (returnValue > 0)
    {
        for (int i = 0; i < 5; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                fprintf(stderr, "1. data received on %d , return value is: %d\n", i, returnValue);
            }
            if (fds[i].revents & POLLRDNORM)
            {
                fprintf(stderr, "2. data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
            if (fds[i].revents & POLLRDBAND)
            {
                fprintf(stderr, "3. data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
            if (fds[i].revents & POLLPRI)
            {
                fprintf(stderr, "4. data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
            if (fds[i].revents & POLLOUT)
            {
                fprintf(stderr, "5data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
            if (fds[i].revents & POLLWRNORM)
            {
                fprintf(stderr, "6data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
            if (fds[i].revents & POLLWRBAND)
            {
                fprintf(stderr, "7data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
            if (fds[i].revents & POLLERR)
            {
                fprintf(stderr, "8data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
            if (fds[i].revents & POLLHUP)
            {
                fprintf(stderr, "9data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
            if (fds[i].revents & POLLNVAL)
            {
                fprintf(stderr, "10data received on %d , return value is: %d\n", fds[i].fd, returnValue);
            }
        }
    }
    else
    {
        fprintf(stderr, "timeout occured\n");
    }

    return lastEvent;
}

eSystemState gotoStateQ1(struct NetNode *netNode, char **argv)
{
    char *eCheckString;
    fprintf(stderr, "entering Q1\n");

    //Read arguments to trackerAddress
    // memset(netNode->UDPAddressA, 0, sizeof(*netNode->UDPAddressA));
    // fprintf(stderr,"memset done\n");
    socklen_t UDPAddressLen = sizeof(netNode->UDPAddressA);
    netNode->UDPAddressA.sin_family = AF_INET;
    netNode->UDPAddressA.sin_port = htons(strtol(argv[2], NULL, 10));
    if (inet_aton(argv[1], &netNode->UDPAddressA.sin_addr) == 0)
    {
        exit_on_error_custom("inet_aton", argv[1]);
    }
    fprintf(stderr, "adress set\n");

    //get the set address in host byte order and print to verify
    eCheckString = inet_ntoa(netNode->UDPAddressA.sin_addr);
    if (eCheckString == NULL)
    {
        exit_on_error("inet_ntoa");
    }
    fprintf(stderr, "tracker address created: %s:%d\n", eCheckString, ntohs(netNode->UDPAddressA.sin_port));

    //Set up udp socket to tracker
    netNode->UDPsocketA = socket(AF_INET, SOCK_DGRAM, 0);
    if (netNode->UDPsocketA == -1)
    {
        exit_on_error("socket udp tracker");
    }
    fprintf(stderr, "socket created\n");

    //Send STUN_LOOKUP
    struct STUN_LOOKUP_PDU lookupMessage = {STUN_LOOKUP};
    socklen_t lookupSize = sizeof(lookupMessage);

    if (sendto(netNode->UDPsocketA, &lookupMessage, lookupSize, 0, (struct sockaddr *)&netNode->UDPAddressA, UDPAddressLen) == -1)
    {
        exit_on_error("send stun lookup error");
    }
    fprintf(stderr, "stun lookup sent: %d\n", lookupMessage.type);

    return q1;
}

eSystemState gotoStateQ2(void)
{
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

eSystemState gotoStateQ3(void)
{
    // //Init self address
    // struct NET_GET_NODE_PDU getNodeMessage = {NET_GET_NODE};
    // socklen_t getNodeSize = sizeof(getNodeMessage);

    // if (sendto(UDPSocketA, &getNodeMessage, getNodeSize, 0, (struct sockaddr *) &trackerAddress, trackerAddressLen) == -1) {
    //     exit_on_error("send get node error");
    // }
    // fprintf(stderr, "[Q3] get node sent\n");

    return q3;
}

eSystemState gotoStateQ4(void)
{
    return q4;
}

eSystemState gotoStateQ5(void)
{
    return q5;
}

eSystemState gotoStateQ6(void)
{
    return q6;
}

eSystemState gotoStateQ7(void)
{
    return q7;
}

eSystemState gotoStateQ8(void)
{
    return q8;
}

eSystemState gotoStateQ9(void)
{
    return q9;
}

eSystemState gotoStateQ10(void)
{
    return q10;
}

eSystemState gotoStateQ11(void)
{
    return q11;
}

eSystemState gotoStateQ12(void)
{
    return q12;
}

eSystemState gotoStateQ13(void)
{
    return q13;
}

eSystemState gotoStateQ14(void)
{
    return q14;
}

eSystemState gotoStateQ15(void)
{
    return q15;
}

eSystemState gotoStateQ16(void)
{
    return q16;
}

eSystemState gotoStateQ17(void)
{
    return q17;
}

eSystemState gotoStateQ18(void)
{
    return q18;
}

eSystemState exitState(void)
{
    return lastState;
}
