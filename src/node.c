#include "node.h"

#define BUFF_SIZE 1024

static void check_params(int argc);
static void exit_on_error(const char *title);
static void exit_on_error_custom(const char *title, const char *detail);
static eSystemEvent readEvent(struct NetNode *netnode);
static eSystemEvent find_right_event(struct NetNode *netNode, unsigned char *buffer, ssize_t buffSize);
static void print_buffer(unsigned char *buffer, int size);
static void fprint_address(struct NetNode *netNode, int socket);
static bool nodeConnected(struct NetNode *netNode);
static void initTCPSocketC(struct NetNode *netNode);
static void writeNetJoinResponse(unsigned char *destMessage, struct NetNode *netNode, struct sockaddr_in nextAddr);
static void writeNetJoinMessage(unsigned char *destMessage, struct NetNode *netNode);
static struct NET_JOIN_PDU readNetJoinMessage(unsigned char *message);
static struct NET_JOIN_RESPONSE_PDU readNetJoinResponse(unsigned char *message);
static struct NET_GET_NODE_RESPONSE_PDU readNetGetNodeResponse(unsigned char *message);
static void fprint_all_sockets(struct NetNode *netNode);

static afEventHandler stateMachine = {
	[q1] = {[eventDone] = gotoStateQ2},			//Q2
	[q2] = {[eventStunResponse] = gotoStateQ3}, //Q3
	[q3] = {[eventNodeResponse] = gotoStateQ7, [eventNodeResponseEmpty] = gotoStateQ4},
	[q4] = {[eventDone] = gotoStateQ6},																																																				  //Q6
	[q5] = {[eventDone] = gotoStateQ6},																																																				  //Q6
	[q6] = {[eventInsertLookupRemove] = gotoStateQ9, [eventShutDown] = gotoStateQ10, [eventJoin] = gotoStateQ12, [eventNewRange] = gotoStateQ15, [eventLeaving] = gotoStateQ16, [eventCloseConnection] = gotoStateQ17, [eventTimeout] = gotoStateQ6}, //Q9
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
	netNode.pduMessage = calloc(BUFF_SIZE, sizeof(unsigned char));
	if (netNode.pduMessage == NULL) {
		exit_on_error("calloc error");
	}

	eSystemState nextState = gotoStateQ1(&netNode, argv);
	eSystemEvent newEvent;

	while (nextState < q13)
	{
		//No events for these states
		if (nextState == q1 || nextState == q4 || nextState == q8 || nextState == q5) 
		{
			fprintf(stderr, "event done\n");
			newEvent = eventDone;
		}
		else if (nextState == q12)
		{
			if (!nodeConnected(&netNode)) //node not connected
			{
				fprintf(stderr, "event not connected\n");
				newEvent = eventNotConnected;
			}
			else 
			{
				//node = max_node
				if ((netNode.nodeRange.max - netNode.nodeRange.min) == netNode.pduMessage[7])
				{
					fprintf(stderr, "event max node\n");
					newEvent = eventMaxNode;
				}
				else //node != max_node
				{
					fprintf(stderr, "event not max node\n");
					newEvent = eventNotMaxNode;
				}
			}
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
	free(netNode.pduMessage);
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
	int timeoutMs = 15000;
	int returnValue;
	unsigned char buffer[BUFF_SIZE];
	memset(buffer, 0, BUFF_SIZE);
	ssize_t bytesRead;

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
				bytesRead = read(netNode->fds[i].fd, buffer, BUFF_SIZE);
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
				// bytesRead = read(netNode->fds[i].fd, buffer, BUFF_SIZE);
			}
			if (netNode->fds[i].revents & POLLNVAL)
			{
				fprintf(stderr, "10. data received on %d , return value is: %d, fd: %d\n", i, returnValue, netNode->fds[i].fd);
			}
		}

		eSystemEvent nextEvent = find_right_event(netNode, buffer, bytesRead); //VAD HÄNDER OM DET FINNS FLER MEDDELANDEN PÅ VÄG?
		return nextEvent;
	}
	else
	{
		fprintf(stderr, "timeout occured\n");
		return eventTimeout;
	}

	return lastEvent;
}

static eSystemEvent find_right_event(struct NetNode *netNode, unsigned char *buffer, ssize_t buffSize)
{
	struct NET_GET_NODE_RESPONSE_PDU getNodeResponse;

	switch (buffer[0]) //Antag att första byten är vilken typ av meddelande vi får
	{
	case STUN_RESPONSE:
		fprintf(stderr, "stun_response received\n");

		memcpy(netNode->pduMessage, buffer, buffSize);
		return eventStunResponse;

	case NET_GET_NODE_RESPONSE:
		fprintf(stderr, "net_get_node_response received\n");

		memcpy(netNode->pduMessage, buffer, buffSize);
		getNodeResponse = readNetGetNodeResponse(netNode->pduMessage);
		if (getNodeResponse.address == 0 && getNodeResponse.port == 0)
		{
			fprintf(stderr, "got empty reponse\n");
			return eventNodeResponseEmpty;
		}
		else
		{
			fprintf(stderr, "got NOT empty response\n");
			return eventNodeResponse;
		}

	case NET_JOIN_RESPONSE:
		fprintf(stderr, "net_join_response received\n");
		
		memcpy(netNode->pduMessage, buffer, buffSize);

		return eventJoinResponse;

	case VAL_INSERT || VAL_LOOKUP || VAL_REMOVE: // FUNKAR DETTA? LOL
		fprintf(stderr, "val insert | val lookup | val remove received\n");
		
		return eventInsertLookupRemove;

	case NET_JOIN:
		fprintf(stderr, "net join received\n");

		memcpy(netNode->pduMessage, buffer, buffSize);

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
	//Set up udp socket to tracker
	netNode->fds[UDP_SOCKET_A].fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (netNode->fds[UDP_SOCKET_A].fd == -1)
	{
		exit_on_error("socket error");
	}

	//Read arguments to trackerAddress
	socklen_t UDPAddressLen = sizeof(netNode->fdsAddr[UDP_SOCKET_A]);
	netNode->fdsAddr[UDP_SOCKET_A].sin_family = AF_INET;
	netNode->fdsAddr[UDP_SOCKET_A].sin_port = htons(strtol(argv[2], NULL, 10));
	if (inet_aton(argv[1], &netNode->fdsAddr[UDP_SOCKET_A].sin_addr) == 0)
	{
		exit_on_error_custom("inet_aton", argv[1]);
	}

	//Send STUN_LOOKUP
	unsigned char lookupMessage[1];
	memset(lookupMessage, 0, sizeof(lookupMessage));
	size_t lookupSize = sizeof(lookupMessage);

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
	if (sendto(netNode->fds[UDP_SOCKET_A].fd, getNodeMessage, messageSize, 0, (struct sockaddr *) &netNode->fdsAddr[UDP_SOCKET_A], addrLength) == -1)
	{
		exit_on_error("send net get node error");
	}
	fprintf(stderr, "net get node sent: ");
	print_buffer(getNodeMessage, messageSize);

	return q3;
}

eSystemState gotoStateQ4(struct NetNode *netNode)
{
	// Vi behöver lista för Q9 och Range för Q15
	netNode->nodeRange.min = 0;
	netNode->nodeRange.max = 0;
	netNode->entries = list_create();

	return q4;
}

eSystemState gotoStateQ5(struct NetNode *netNode)
{
	//Node not connected, connect to successor
	//Connect to src address in message and store connection as succ address
	struct NET_JOIN_PDU joinRequest = readNetJoinMessage(netNode->pduMessage);
	netNode->fdsAddr[TCP_SOCKET_B].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr = joinRequest.src_address;
	netNode->fdsAddr[TCP_SOCKET_B].sin_port = joinRequest.src_port;
	socklen_t addrLenB = sizeof(netNode->fdsAddr[TCP_SOCKET_B]);

	netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_B].fd == -1)
	{
		exit_on_error("socket error");
	}

	if (connect(netNode->fds[TCP_SOCKET_B].fd, (struct sockaddr *) &netNode->fdsAddr[TCP_SOCKET_B], addrLenB) == -1)
	{
		exit_on_error("connect error");
	}

	// Open socket
	if (netNode->fds[TCP_SOCKET_C].fd == 0) //Socket C not initialized yet
	{
		fprintf(stderr, "init TCP C\n");
		initTCPSocketC(netNode);
	}

	//Send NET_JOIN_RESPONSE
	unsigned char netJoinResponseMessage[9];
	size_t messageSize = sizeof(netJoinResponseMessage);
	memset(netJoinResponseMessage, 0, messageSize);
	writeNetJoinResponse(netJoinResponseMessage, netNode, netNode->fdsAddr[TCP_SOCKET_C]);

	if (send(netNode->fds[TCP_SOCKET_B].fd, netJoinResponseMessage, messageSize, 0) == -1)
	{
		exit_on_error("send error");
	}

	//Transfer upper half of entry-range to succ //NEED TO BE DONE

	// Accept predecessor (same as succ)
	if (listen(netNode->fds[TCP_SOCKET_C].fd, 2) == -1)
	{
		exit_on_error("listen error");
	}

	socklen_t addrLenC = sizeof(netNode->fdsAddr[TCP_SOCKET_C]);
	netNode->fds[TCP_SOCKET_D].fd = accept(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *) &netNode->fdsAddr[TCP_SOCKET_C], &addrLenC);
	if (netNode->fds[TCP_SOCKET_D].fd == -1)
	{
		exit_on_error("accept error");
	}
	fprint_all_sockets(netNode);

	return q5;
}

eSystemState gotoStateQ6(struct NetNode *netNode)
{
	//Send NET_ALIVE
	unsigned char netAliveMessage[1];
	size_t messageSize = sizeof(netAliveMessage);
	memset(netAliveMessage, 0, messageSize);
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
	netNode->fds[UDP_SOCKET_A2].fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (netNode->fds[UDP_SOCKET_A2].fd == -1)
	{
		exit_on_error("socket error");
	}

	//init UDP A2 address
	struct NET_GET_NODE_RESPONSE_PDU getNodeResponse = readNetGetNodeResponse(netNode->pduMessage);
	netNode->fdsAddr[UDP_SOCKET_A2].sin_family = AF_INET;
	netNode->fdsAddr[UDP_SOCKET_A2].sin_addr.s_addr = getNodeResponse.address;
	netNode->fdsAddr[UDP_SOCKET_A2].sin_port = getNodeResponse.port;

	initTCPSocketC(netNode);

	//Send NET_JOIN
	unsigned char netJoinMessage[14];
	size_t messageSize = sizeof(netJoinMessage);
	memset(netJoinMessage, 0, messageSize);
	writeNetJoinMessage(netJoinMessage, netNode);
	socklen_t udpAddressLen = sizeof(netNode->fdsAddr[UDP_SOCKET_A2]);

	if (sendto(netNode->fds[UDP_SOCKET_A2].fd, netJoinMessage, messageSize, 0, (struct sockaddr *) &netNode->fdsAddr[UDP_SOCKET_A2], udpAddressLen) == -1)
	{
		exit_on_error("send net join error");
	}
	fprintf(stderr, "net join sent: %d\n", netJoinMessage[0]);

	if (listen(netNode->fds[TCP_SOCKET_C].fd, 1) == -1)
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
	netNode->entries = list_create();

	netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_B].fd == -1)
	{
		exit_on_error("socket error");
	}

	struct NET_JOIN_RESPONSE_PDU joinResponse = readNetJoinResponse(netNode->pduMessage);
	netNode->fdsAddr[TCP_SOCKET_B].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr = joinResponse.next_address;
	netNode->fdsAddr[TCP_SOCKET_B].sin_port = joinResponse.next_port;
	socklen_t addrLenB = sizeof(netNode->fdsAddr[TCP_SOCKET_B]);
	
	print_buffer(netNode->pduMessage, 9);
	fprint_all_sockets(netNode);

	// fprint_address(netNode, TCP_SOCKET_B);
	if (connect(netNode->fds[TCP_SOCKET_B].fd, (struct sockaddr *) &netNode->fdsAddr[TCP_SOCKET_B], addrLenB) == -1)
	{
		exit_on_error("connect error");
	}
	fprintf(stderr, "connect done\n");

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

eSystemState gotoStateQ12(struct NetNode *netNode) //On NET_JOIN
{
	//Read message as NET_JOIN_PDU
	struct NET_JOIN_PDU joinRequest = readNetJoinMessage(netNode->pduMessage);

	//Update PDU max fields
	unsigned char range = (netNode->nodeRange.max - netNode->nodeRange.min);
	if (range > joinRequest.max_span)
	{
		//New max node, update max_span, max_address, max_port
		memcpy(&netNode->pduMessage[7], &range, 1);
		memcpy(&netNode->pduMessage[8], &netNode->fdsAddr[TCP_SOCKET_C].sin_addr.s_addr, 4);
		memcpy(&netNode->pduMessage[12], &netNode->fdsAddr[TCP_SOCKET_C].sin_port, 2);
	}

	return q12;
}

eSystemState gotoStateQ13(struct NetNode *netNode)
{
	//Node is max_node

	//Store address to old successor
	struct sockaddr_in oldSuccessor = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr,
		.sin_port = netNode->fdsAddr[TCP_SOCKET_B].sin_port};

	//Send NET_CLOSE_CONNECTION to successor
	unsigned char closeConnectionMessage[1];
	size_t messageCloseSize = sizeof(closeConnectionMessage);
	memset(closeConnectionMessage, 0, messageCloseSize);
	closeConnectionMessage[0] = NET_CLOSE_CONNECTION;

	if (send(netNode->fds[TCP_SOCKET_B].fd, closeConnectionMessage, messageCloseSize, 0) == -1)
	{
		exit_on_error("send error");
	}

	//Close and reopen socket B
	close(netNode->fds[TCP_SOCKET_B].fd);
	netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_B].fd == -1)
	{
		exit_on_error("socket error");
	}

	//Connect to prospect
	struct NET_JOIN_PDU joinRequest = readNetJoinMessage(netNode->pduMessage);
	netNode->fdsAddr[TCP_SOCKET_B].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr = joinRequest.src_address;
	netNode->fdsAddr[TCP_SOCKET_B].sin_port = joinRequest.src_port;
	socklen_t addLenB = sizeof(netNode->fdsAddr[TCP_SOCKET_B]);
	if (connect(netNode->fds[TCP_SOCKET_B].fd, (struct sockaddr *) &netNode->fdsAddr[TCP_SOCKET_B], addLenB) == -1)
	{
		exit_on_error("connect error");
	}

	//Send NET_JOIN_RESPONSE to prospect
	unsigned char netJoinResponseMessage[9];
	size_t messageJoinSize = sizeof(netJoinResponseMessage);
	memset(netJoinResponseMessage, 0, messageJoinSize);
	writeNetJoinResponse(netJoinResponseMessage, netNode, oldSuccessor);

	if (send(netNode->fds[TCP_SOCKET_B].fd, netJoinResponseMessage, messageJoinSize, 0) == -1)
	{
		exit_on_error("send error");
	}
	
	//Transfer upper half of entry-range to succ //NEED TO BE IMPLEMENTED

	return q13;
}

eSystemState gotoStateQ14(struct NetNode *netNode)
{
	//If node != max_node //Q14
	//Forward NET_JOIN
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

static void print_buffer(unsigned char *buffer, int size)
{
	for (int c = 0; c < size; c++)
	{
		fprintf(stderr, "%d ", buffer[c]);
	}
	fprintf(stderr, "\n");
}

static void fprint_address(struct NetNode *netNode, int socket)
{
	if (socket == UDP_SOCKET_A)
	{
		fprintf(stderr, "UDP A: ");
	}
	else if (socket == TCP_SOCKET_B)
	{
		fprintf(stderr, "TCP B: ");
	}
	else if (socket == TCP_SOCKET_C)
	{
		fprintf(stderr, "TCP C: ");
	}
	else if (socket == TCP_SOCKET_D)
	{
		fprintf(stderr, "TCP D: ");
	}
	else if (socket == UDP_SOCKET_A2)
	{
		fprintf(stderr, "UDP A2: ");
	}
	fprintf(stderr, "%s:%d\n", inet_ntoa(netNode->fdsAddr[socket].sin_addr), ntohs(netNode->fdsAddr[socket].sin_port));
}

static bool nodeConnected(struct NetNode *netNode)
{
	//If socket D or B has been opened the node is connected
	return netNode->fds[TCP_SOCKET_D].fd != 0 || netNode->fds[TCP_SOCKET_B].fd != 0;
}

static void initTCPSocketC(struct NetNode *netNode)
{
	netNode->fds[TCP_SOCKET_C].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_C].fd == -1)
	{
		exit_on_error("socket error");
	}

	//init TCP C address
	memset(&netNode->fdsAddr[TCP_SOCKET_C], 0, sizeof(netNode->fdsAddr[TCP_SOCKET_C]));
	netNode->fdsAddr[TCP_SOCKET_C].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_C].sin_addr.s_addr = htonl(INADDR_ANY);
	netNode->fdsAddr[TCP_SOCKET_C].sin_port = 0;

	int reuseaddr = 1;
	if (setsockopt(netNode->fds[TCP_SOCKET_C].fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
	{
		exit_on_error("setsockopt error");
	}

	socklen_t addrLengthC = sizeof(netNode->fdsAddr[TCP_SOCKET_C]);
	if (bind(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_C], addrLengthC) == -1)
	{
		exit_on_error("bind error TCP C");
	}

	if (getsockname(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_C], &addrLengthC) == -1)
	{
		exit_on_error("getsockname error");
	}
	fprintf(stderr, "TCP C: %s:%d\n", inet_ntoa(netNode->fdsAddr[TCP_SOCKET_C].sin_addr), ntohs(netNode->fdsAddr[TCP_SOCKET_C].sin_port));
}

static struct NET_GET_NODE_RESPONSE_PDU readNetGetNodeResponse(unsigned char *message)
{
	struct NET_GET_NODE_RESPONSE_PDU getNodeResponse;
	memcpy(&getNodeResponse.type, &message[0], 1);
	memcpy(&getNodeResponse.address, &message[1], 4);
	memcpy(&getNodeResponse.port, &message[5], 2);

	return getNodeResponse;
}

static struct NET_JOIN_PDU readNetJoinMessage(unsigned char *message)
{
	//Read message as NET_JOIN_PDU
	struct NET_JOIN_PDU joinRequest;
	memcpy(&joinRequest.type, &message[0], 1);
	memcpy(&joinRequest.src_address, &message[1], 4);
	memcpy(&joinRequest.src_port, &message[5], 2);
	memcpy(&joinRequest.max_span, &message[7], 1);
	memcpy(&joinRequest.max_address, &message[8], 4);
	memcpy(&joinRequest.max_port, &message[12], 2);
	return joinRequest;
}

static struct NET_JOIN_RESPONSE_PDU readNetJoinResponse(unsigned char *message)
{
	//Read message as NET_JOIN_RESPONSE_PDU
	struct NET_JOIN_RESPONSE_PDU joinResponse;
	memcpy(&joinResponse.type, &message[0], 1);
	memcpy(&joinResponse.next_address, &message[1], 4);
	memcpy(&joinResponse.next_port, &message[5], 2);
	memcpy(&joinResponse.range_start, &message[7], 1);
	memcpy(&joinResponse.range_end, &message[8], 1);
	return joinResponse;
}

static void writeNetJoinMessage(unsigned char *destMessage, struct NetNode *netNode)
{
	struct NET_JOIN_PDU netJoin = {NET_JOIN,
								   netNode->fdsAddr[TCP_SOCKET_C].sin_addr.s_addr, //Src address
								   netNode->fdsAddr[TCP_SOCKET_C].sin_port,		   //Src port
								   (netNode->nodeRange.max - netNode->nodeRange.min), //Max span
								   netNode->fdsAddr[TCP_SOCKET_C].sin_addr.s_addr, //Max address
								   netNode->fdsAddr[TCP_SOCKET_C].sin_port};	   //Max port

	memcpy(&destMessage[0], &netJoin.type, 1);
	memcpy(&destMessage[1], &netJoin.src_address, 4);
	memcpy(&destMessage[5], &netJoin.src_port, 2);
	memcpy(&destMessage[7], &netJoin.max_span, 1);
	memcpy(&destMessage[8], &netJoin.max_address, 4);
	memcpy(&destMessage[12], &netJoin.max_port, 2);
}

static void writeNetJoinResponse(unsigned char *destMessage, struct NetNode *netNode, struct sockaddr_in nextAddr)
{
	/* 
	Calculate new hash range
	predecessor = node in response
	successor = this node
	minP = minP
	maxS = maxP
	maxP = floor( (maxP - minP) / 2 ) + minP
	minS = maxP + 1 
	*/

	unsigned char minP = netNode->nodeRange.min;
	unsigned char maxP = netNode->nodeRange.max;
	unsigned char maxS = maxP;
	maxP = (unsigned char)((maxP - minP) / 2) + minP;
	unsigned char minS = maxP + 1;

	destMessage[0] = NET_JOIN_RESPONSE;
	memcpy(&destMessage[1], &nextAddr.sin_addr.s_addr, 4);
	memcpy(&destMessage[5], &nextAddr.sin_port, 2);
	memcpy(&destMessage[7], &minS, 1);
	memcpy(&destMessage[8], &maxS, 1);
	// fprintf(stderr, "minS: %d maxS: %d\n", minS, maxS);
	// fprintf(stderr, "minP: %d maxP: %d\n", netNode->nodeRange.min, netNode->nodeRange.max);
}

static void fprint_all_sockets(struct NetNode *netNode)
{
	fprintf(stderr, "fd: %d ", netNode->fds[UDP_SOCKET_A].fd);
	fprint_address(netNode, UDP_SOCKET_A);
	
	fprintf(stderr, "fd: %d ", netNode->fds[TCP_SOCKET_B].fd);
	fprint_address(netNode, TCP_SOCKET_B);
	
	fprintf(stderr, "fd: %d ", netNode->fds[TCP_SOCKET_C].fd);
	fprint_address(netNode, TCP_SOCKET_C);
	
	fprintf(stderr, "fd: %d ", netNode->fds[TCP_SOCKET_D].fd);
	fprint_address(netNode, TCP_SOCKET_D);

	fprintf(stderr, "fd: %d ", netNode->fds[UDP_SOCKET_A2].fd);
	fprint_address(netNode, UDP_SOCKET_A2);
}