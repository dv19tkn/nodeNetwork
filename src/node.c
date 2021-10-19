#include "node.h"

#define BUFF_SIZE 1024

static void check_params(int argc);
static void exit_on_error(const char *title);
static void exit_on_error_custom(const char *title, const char *detail);
static eSystemEvent readEvent(struct NetNode *netnode);
static eSystemEvent find_right_event(struct NetNode *netNode, unsigned char *buffer);

static afEventHandler stateMachine = {
	[q1] = {[eventDone] = gotoStateQ2},			//Q2
	[q2] = {[eventStunResponse] = gotoStateQ3}, //Q3
	[q3] = {[eventNodeResponse] = gotoStateQ7, [eventNodeResponseEmpty] = gotoStateQ4},
	[q4] = {[eventDone] = gotoStateQ6},																																													//Q6
	[q5] = {[eventDone] = gotoStateQ6},																																													//Q6
	[q6] = {[eventInsertLookupRemove] = gotoStateQ9, [eventShutDown] = gotoStateQ10, [eventJoin] = gotoStateQ12, [eventNewRange] = gotoStateQ15, [eventLeaving] = gotoStateQ16, [eventCloseConnection] = gotoStateQ17}, //Q9
	[q7] = {[eventJoinResponse] = gotoStateQ8},
	[q8] = {[eventDone] = gotoStateQ6},
	[q9] = {[eventDone] = gotoStateQ6},																			  //Q6
	[q10] = {[eventConnected] = gotoStateQ11, [eventNotConnected] = exitState},									  //Q11
	[q11] = {[eventNewRangeResponse] = gotoStateQ18},															  //Q18
	[q12] = {[eventNotConnected] = gotoStateQ5, [eventMaxNode] = gotoStateQ13, [eventNotMaxNode] = gotoStateQ14}, //Q5
	[q18] = {[eventDone] = exitState},																			  //exit
	[q13] = {[eventDone] = gotoStateQ6},																		  //Q6
	[q14] = {[eventDone] = gotoStateQ6}																			  //Q6
};

int main(int argc, char **argv)
{
	check_params(argc);

	struct NetNode netNode = {};
	memset(&netNode, 0, sizeof(netNode));

	eSystemState nextState = gotoStateQ1(&netNode, argv);
	// nextState = gotoStateQ2(&netNode);
	eSystemEvent newEvent;

	while (nextState != q6)
	{
		// fprintf(stderr, "current state is: Q%d \n", nextState + 1);

		if (nextState == q1 || nextState == q4 || nextState == q8) //No events for these states
		{
			newEvent = eventDone;
		}
		else if (nextState == q12)
		{
			if (true) //node not connected
			{
				newEvent = eventNotConnected;
			}
			else if (true) // node = max_node
			{
			}
			else // node != max_node
			{
			}
			newEvent = eventDone;
		}
		else if (nextState == q10)
		{
			if (true) // node is connected
			{
				newEvent = eventNotConnected; //BORDE VARA CONNECTED
			}
			else // node is not connected
			{
				exit(1);
			}
		}
		else
		{
			newEvent = readEvent(&netNode);
		}

		if ((nextState < lastState) && (newEvent < lastEvent) && stateMachine[nextState][newEvent] != NULL)
		{
			nextState = (*stateMachine[nextState][newEvent])(&netNode);
			fprintf(stderr, "current state: Q%d\n", nextState + 1);
		}
		else
		{
			fprintf(stderr, "invalid state Q%d or event E%d\n", nextState + 1, newEvent + 1);
			break;
		}
	}

	close(netNode.fds[UDP_SOCKET_A].fd);
	close(netNode.fds[TCP_SOCKET_B].fd);
	close(netNode.fds[TCP_SOCKET_C].fd);
	close(netNode.fds[TCP_SOCKET_D].fd);
	if (netNode.entries != NULL)
	{
		list_destroy(netNode.entries);
	}
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

static eSystemEvent readEvent(struct NetNode *netNode)
{
	int timeoutMs = 2000;
	int returnValue;
	unsigned char buffer[BUFF_SIZE];

	netNode->fds[UDP_SOCKET_A].events = POLLIN; // | POLLOUT;
	netNode->fds[TCP_SOCKET_B].events = POLLIN; // | POLLOUT;
	netNode->fds[TCP_SOCKET_C].events = POLLIN;
	netNode->fds[TCP_SOCKET_D].events = POLLIN;	 // | POLLOUT;
	netNode->fds[UDP_SOCKET_A2].events = POLLIN; // | POLLOUT;

	returnValue = poll(netNode->fds, NO_SOCKETS, timeoutMs);

	if (returnValue == -1)
	{
		exit_on_error("poll error");
	}

	if (returnValue > 0)
	{
		for (int i = 0; i < NO_SOCKETS; i++)
		{
			if (netNode->fds[i].revents & POLLIN)
			{
				fprintf(stderr, "1. data received on %d, return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
				read(netNode->fds[i].fd, buffer, BUFF_SIZE);
			}
			if (netNode->fds[i].revents & POLLRDNORM)
			{
				fprintf(stderr, "2. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
			if (netNode->fds[i].revents & POLLRDBAND)
			{
				fprintf(stderr, "3. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
			if (netNode->fds[i].revents & POLLPRI)
			{
				fprintf(stderr, "4. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
			if (netNode->fds[i].revents & POLLOUT)
			{
				fprintf(stderr, "5. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
			if (netNode->fds[i].revents & POLLWRNORM)
			{
				fprintf(stderr, "6. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
			if (netNode->fds[i].revents & POLLWRBAND)
			{
				fprintf(stderr, "7. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
			if (netNode->fds[i].revents & POLLERR) //Error
			{
				fprintf(stderr, "8. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
			if (netNode->fds[i].revents & POLLHUP) //Hang up
			{
				fprintf(stderr, "9. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
				read(netNode->fds[i].fd, buffer, BUFF_SIZE);
			}
			if (netNode->fds[i].revents & POLLNVAL)
			{
				fprintf(stderr, "10. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
		}

		eSystemEvent nextEvent = find_right_event(netNode, buffer); //VAD HÄNDER OM DET FINNS FLER MEDDELANDEN PÅ VÄG?
		return nextEvent;
	}
	else
	{
		fprintf(stderr, "timeout occured\n");
	}

	return lastEvent;
}

static eSystemEvent find_right_event(struct NetNode *netNode, unsigned char *buffer)
{
	struct STUN_RESPONSE_PDU response;
	struct NET_GET_NODE_RESPONSE_PDU getNodeResponse;
	struct NET_JOIN_RESPONSE_PDU joinResponse;
	struct sockaddr_in preNodeAddress;

	fprintf(stderr, "buffer[0]: %d\n", buffer[0]);

	switch (buffer[0]) //Antag att första byten är vilken typ av meddelande vi får
	{
	case STUN_RESPONSE:
		fprintf(stderr, "stun_response received\n");
		memcpy(&response.type, &buffer[0], 1);
		memcpy(&response.address, &buffer[1], 4);
		// fprintf(stderr, "type: %d, address: %d\n", response.type, response.address);
		return eventStunResponse;

	case NET_GET_NODE_RESPONSE:
		fprintf(stderr, "net_get_node_response received\n");
		memcpy(&getNodeResponse.type, &buffer[0], 1);
		memcpy(&getNodeResponse.address, &buffer[1], 4);
		memcpy(&getNodeResponse.port, &buffer[5], 2);

		memset(&preNodeAddress, 0, sizeof(preNodeAddress));
		preNodeAddress.sin_family = AF_INET;
		preNodeAddress.sin_port = getNodeResponse.port;
		preNodeAddress.sin_addr.s_addr = getNodeResponse.address;

		if (preNodeAddress.sin_port == 0 && preNodeAddress.sin_addr.s_addr == 0)
		{
			fprintf(stderr, "got empty reponse\n");
			return eventNodeResponseEmpty;
		}
		else
		{
			netNode->fdsAddr[UDP_SOCKET_A2].sin_family = AF_INET;
			netNode->fdsAddr[UDP_SOCKET_A2].sin_port = preNodeAddress.sin_port;
			netNode->fdsAddr[UDP_SOCKET_A2].sin_addr.s_addr = preNodeAddress.sin_addr.s_addr;
			fprintf(stderr, "received address: %s:%d\n", inet_ntoa(preNodeAddress.sin_addr), ntohs(preNodeAddress.sin_port));
			return eventNodeResponse;
		}

	case NET_JOIN_RESPONSE:
		fprintf(stderr, "net_join_response received\n");
		memcpy(&joinResponse.type, &buffer[0], 1);
		memcpy(&joinResponse.next_address, &buffer[1], 4);
		memcpy(&joinResponse.next_port, &buffer[5], 2);
		memcpy(&joinResponse.range_start, &buffer[7], 1);
		memcpy(&joinResponse.range_end, &buffer[8], 1);

		netNode->fdsAddr[TCP_SOCKET_B].sin_family = AF_INET;
		netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr = joinResponse.next_address;
		netNode->fdsAddr[TCP_SOCKET_B].sin_port = joinResponse.next_port;

		/* 
		Calculate new hash range
		predecessor = node in response
		successor = this node
		minP = minP
		maxS = maxP
		maxP = floor( (maxP - minP) / 2 ) + minP
		minS = maxP + 1 
		*/
		int minP = joinResponse.range_start;
		int maxP = joinResponse.range_end;
		int maxS = maxP;
		maxP = (int)((maxP - minP) / 2) + minP;
		int minS = maxP + 1;

		fprintf(stderr, "minS: %d maxS: %d\n", minS, maxS);
		fprintf(stderr, "minP: %d maxP: %d\n", minP, maxP);

		netNode->nodeRange.min = minS;
		netNode->nodeRange.max = maxS;

		return eventJoinResponse;

	case VAL_INSERT || VAL_LOOKUP || VAL_REMOVE: // FUNKAR DETTA? LOL
		fprintf(stderr, "val insert | val lookup | val remove received\n");
		return eventInsertLookupRemove;

	case NET_JOIN:
		fprintf(stderr, "net join received\n");
		return eventJoin;

	case NET_NEW_RANGE:
		fprintf(stderr, "net new range received\n");
		return eventNewRange;

	case NET_LEAVING:
		fprintf(stderr, "net leaving received\n");
		return eventLeaving;

	case NET_CLOSE_CONNECTION:
		fprintf(stderr, "net close connection received\n");
		return eventCloseConnection;

	case NET_NEW_RANGE_RESPONSE:
		fprintf(stderr, "net new range received\n");
		return eventNewRangeResponse;

	default:
		fprintf(stderr, "unknown response\n");
		return lastEvent; //ÄNDRA DETTA(?)
	}
}

eSystemState gotoStateQ1(struct NetNode *netNode, char **argv)
{
	char *eCheckString;

	//Set up udp socket to tracker
	netNode->fds[UDP_SOCKET_A].fd = socket(AF_INET, SOCK_DGRAM, 0);
	// netNode->fds[UDP_SOCKET_A].events = POLLIN; // | POLLOUT;
	// netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
	// netNode->fds[TCP_SOCKET_C].fd = socket(AF_INET, SOCK_STREAM, 0);
	// netNode->fds[TCP_SOCKET_D].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[UDP_SOCKET_A].fd == -1) // || netNode->fds[TCP_SOCKET_B].fd == -1 ||
											 // netNode->fds[TCP_SOCKET_C].fd == -1 || netNode->fds[TCP_SOCKET_D].fd == -1)
	{
		exit_on_error("socket error");
	}
	// fprintf(stderr, "socket created fd: %d\n", netNode->fds[UDP_SOCKET_A].fd);

	//Read arguments to trackerAddress
	socklen_t UDPAddressLen = sizeof(netNode->fdsAddr[UDP_SOCKET_A]);
	netNode->fdsAddr[UDP_SOCKET_A].sin_family = AF_INET;
	netNode->fdsAddr[UDP_SOCKET_A].sin_port = htons(strtol(argv[2], NULL, 10));
	if (inet_aton(argv[1], &netNode->fdsAddr[UDP_SOCKET_A].sin_addr) == 0)
	{
		exit_on_error_custom("inet_aton", argv[1]);
	}
	// fprintf(stderr, "adress set\n");

	//get the set address in host byte order and print to verify
	eCheckString = inet_ntoa(netNode->fdsAddr[UDP_SOCKET_A].sin_addr);
	if (eCheckString == NULL)
	{
		exit_on_error("inet_ntoa");
	}
	// fprintf(stderr, "tracker address created: %s:%d\n", eCheckString, ntohs(netNode->fdsAddr[UDP_SOCKET_A].sin_port));

	//Send STUN_LOOKUP
	unsigned char lookupMessage[1];
	memset(lookupMessage, 0, sizeof(lookupMessage));
	size_t lookupSize = sizeof(lookupMessage);
	// fprintf(stderr, "size message: %ld\n", lookupSize);

	lookupMessage[0] = STUN_LOOKUP;

	if (sendto(netNode->fds[UDP_SOCKET_A].fd, lookupMessage, lookupSize, 0, (struct sockaddr *)&netNode->fdsAddr[UDP_SOCKET_A], UDPAddressLen) == -1)
	{
		exit_on_error("send stun lookup error");
	}
	fprintf(stderr, "stun lookup sent: %d\n", lookupMessage[0]);

	return q1;
}

eSystemState gotoStateQ2(struct NetNode *netNode)
{
	return q2;
}

eSystemState gotoStateQ3(struct NetNode *netNode)
{
	//Send NET_GET_NODE
	unsigned char getNodeMessage[1];
	memset(getNodeMessage, 0, sizeof(getNodeMessage));
	size_t messageSize = sizeof(getNodeMessage);
	socklen_t addrLength = sizeof(netNode->fdsAddr[UDP_SOCKET_A]);

	getNodeMessage[0] = NET_GET_NODE;
	if (sendto(netNode->fds[UDP_SOCKET_A].fd, getNodeMessage, messageSize, 0, (struct sockaddr *)&netNode->fdsAddr[UDP_SOCKET_A], addrLength) == -1)
	{
		exit_on_error("send net get node error");
	}
	fprintf(stderr, "net get node sent: %d\n", getNodeMessage[0]);

	return q3;
}

eSystemState gotoStateQ4(struct NetNode *netNode)
{
	// Vi behöver lista för Q9 och Range för Q15
	netNode->nodeRange.min = 0;
	netNode->nodeRange.max = 255;
	netNode->entries = list_create();

	return q4;
}

eSystemState gotoStateQ5(struct NetNode *netNode)
{
	return q5;
}

eSystemState gotoStateQ6(struct NetNode *netNode)
{
	//Send NET_ALIVE
	unsigned char netAliveMessage[1];
	memset(netAliveMessage, 0, sizeof(netAliveMessage));
	size_t messageSize = sizeof(netAliveMessage);
	socklen_t addrLength = sizeof(netNode->fdsAddr[UDP_SOCKET_A]);

	netAliveMessage[0] = NET_ALIVE;
	if (sendto(netNode->fds[UDP_SOCKET_A].fd, netAliveMessage, messageSize, 0, (struct sockaddr *)&netNode->fdsAddr[UDP_SOCKET_A], addrLength) == -1)
	{
		exit_on_error("send net alive error");
	}
	fprintf(stderr, "net alive sent: %d\n", netAliveMessage[0]);

	return q6;
}

eSystemState gotoStateQ7(struct NetNode *netNode)
{
	netNode->fds[TCP_SOCKET_C].fd = socket(AF_INET, SOCK_STREAM, 0);
	// netNode->fds[TCP_SOCKET_C].events = POLLIN | POLLOUT;

	netNode->fds[UDP_SOCKET_A2].fd = socket(AF_INET, SOCK_DGRAM, 0);
	// netNode->fds[UDP_SOCKET_A2].events = POLLIN;
	if (netNode->fds[UDP_SOCKET_A2].fd == -1 || netNode->fds[TCP_SOCKET_C].fd == -1)
	{
		exit_on_error("socket error");
	}

	//init TCP C address
	memset(&netNode->fdsAddr[TCP_SOCKET_C], 0, sizeof(netNode->fdsAddr[TCP_SOCKET_C]));
	netNode->fdsAddr[TCP_SOCKET_C].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_C].sin_addr.s_addr = htonl(INADDR_ANY);
	netNode->fdsAddr[TCP_SOCKET_C].sin_port = 0;

	int reuseaddr=1;
	if (setsockopt(netNode->fds[TCP_SOCKET_C].fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) 
	{
    	exit_on_error("setsockopt error");
	}	

	socklen_t addrLengthC = sizeof(netNode->fdsAddr[TCP_SOCKET_C]);
	if (bind(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_C], addrLengthC) == -1)
	{
		exit_on_error("bind error TCP C");
	}

	if (getsockname(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *) &netNode->fdsAddr[TCP_SOCKET_C], &addrLengthC) == -1)
	{
		exit_on_error("getsockname error");
	}
	fprintf(stderr, "TCP C: %s:%d\n", inet_ntoa(netNode->fdsAddr[TCP_SOCKET_C].sin_addr), ntohs(netNode->fdsAddr[TCP_SOCKET_C].sin_port));

	//Send NET_JOIN
	unsigned char netJoinMessage[14];
	memset(netJoinMessage, 0, sizeof(netJoinMessage));
	size_t messageSize = sizeof(netJoinMessage);

	//OKLART OM DETTA ÄR VAD SOM SKA SKICKAS
	struct NET_JOIN_PDU netJoin = {NET_JOIN,
								   netNode->fdsAddr[TCP_SOCKET_C].sin_addr.s_addr, //Src address
								   netNode->fdsAddr[TCP_SOCKET_C].sin_port,		   //Src port
								   255,											   //Max span
								   netNode->fdsAddr[UDP_SOCKET_A2].sin_addr.s_addr, //Max address
								   netNode->fdsAddr[UDP_SOCKET_A2].sin_port};	   //Max port

	memcpy(&netJoinMessage[0], &netJoin.type, 1);
	memcpy(&netJoinMessage[1], &netJoin.src_address, 4);
	memcpy(&netJoinMessage[5], &netJoin.src_port, 2);
	memcpy(&netJoinMessage[7], &netJoin.max_span, 1);
	memcpy(&netJoinMessage[8], &netJoin.max_address, 4);
	memcpy(&netJoinMessage[12], &netJoin.max_port, 2);
	for (int c = 0; c < 14; c++) {
		fprintf(stderr, "%d ", netJoinMessage[c]);
	}
	fprintf(stderr, "\n");

	socklen_t udpAddressLen = sizeof(netNode->fdsAddr[UDP_SOCKET_A2]);
	if (sendto(netNode->fds[UDP_SOCKET_A2].fd, netJoinMessage, messageSize, 0, (struct sockaddr *) &netNode->fdsAddr[UDP_SOCKET_A2], udpAddressLen) == -1)
	{
		exit_on_error("send net join error");
	}
	fprintf(stderr, "net join sent: %d\n", netJoinMessage[0]);

	if (listen(netNode->fds[TCP_SOCKET_C].fd, 2) == -1)
	{
		exit_on_error("listen error");
	}
	fprintf(stderr, "listen done\n");
	
	socklen_t preNodeLen = sizeof(netNode->fdsAddr[TCP_SOCKET_D]);
	netNode->fds[TCP_SOCKET_D].fd = accept(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *) &netNode->fdsAddr[TCP_SOCKET_D], &preNodeLen);
	if (netNode->fds[TCP_SOCKET_D].fd == -1)
	{
		exit_on_error("accept error");
	}
	fprintf(stderr, "accept done\n");

	return q7;
}

eSystemState gotoStateQ8(struct NetNode *netNode)
{
	netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_B].fd == -1)
	{
		exit_on_error("socket error");
	}

	netNode->entries = list_create();
	socklen_t addrLengthB = sizeof(netNode->fdsAddr[TCP_SOCKET_B]);

	int reuseaddr=1;
	if (setsockopt(netNode->fds[TCP_SOCKET_B].fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) 
	{
    	exit_on_error("setsockopt error");
	}	

	if (bind(netNode->fds[TCP_SOCKET_B].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_B], addrLengthB) == -1)
	{
		exit_on_error("bind error TCP B");
	}
	// fprintf(stderr, "TCP B: %s:%d\n", inet_ntoa(netNode->fdsAddr[TCP_SOCKET_B].sin_addr), ntohs(netNode->fdsAddr[TCP_SOCKET_B].sin_port));

	return q8;
}

eSystemState gotoStateQ9(struct NetNode *netNode)
{
	return q9;
}

eSystemState gotoStateQ10(struct NetNode *netNode)
{
	return q10;
}

eSystemState gotoStateQ11(struct NetNode *netNode)
{
	return q11;
}

eSystemState gotoStateQ12(struct NetNode *netNode)
{
	return q12;
}

eSystemState gotoStateQ13(struct NetNode *netNode)
{
	return q13;
}

eSystemState gotoStateQ14(struct NetNode *netNode)
{
	return q14;
}

eSystemState gotoStateQ15(struct NetNode *netNode)
{
	return q15;
}

eSystemState gotoStateQ16(struct NetNode *netNode)
{
	return q16;
}

eSystemState gotoStateQ17(struct NetNode *netNode)
{
	return q17;
}

eSystemState gotoStateQ18(struct NetNode *netNode)
{
	return q18;
}

eSystemState exitState(struct NetNode *netNode)
{
	return lastState;
}

void send_message_UDP(struct NetNode *netNode, int message, char *messageText, int UDP_socket_index)
{
	unsigned char messageBuffer[1];
	memset(messageBuffer, 0, sizeof(messageBuffer));
	size_t messageSize = sizeof(messageBuffer);
	socklen_t addrLength = sizeof(netNode->fdsAddr[UDP_socket_index]);

	messageBuffer[0] = message;
	if (sendto(netNode->fds[UDP_socket_index].fd, messageBuffer, messageSize, 0, (struct sockaddr *)&netNode->fdsAddr[UDP_socket_index], addrLength) == -1)
	{
		char errorMsg[200] = {"Error: "};
		strcat(errorMsg, messageText);
		exit_on_error(errorMsg);
	}
	// fprintf(stderr, "%s sent: %d\n", messageText, messageBuffer[0]);
}

void send_message_TCP()
{
}