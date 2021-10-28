#include "node.h"

#define BUFF_SIZE 8192

void sig_handler(int signum);

static void check_params(int argc);
static void exit_on_error(const char *title, struct NetNode *netNode);
static void exit_on_error_custom(const char *title, const char *detail);
static eSystemEvent readEvent(struct NetNode *netNode, eSystemState state);
static eSystemEvent readFromSockets(struct NetNode *netnode);
static eSystemEvent findRightEvent(struct NetNode *netNode, unsigned char *buffer, ssize_t buffSize);
static bool nodeConnected(struct NetNode *netNode);
static void initTCPSocketC(struct NetNode *netNode);
static void writeNetJoinResponse(unsigned char *destMessage, struct NetNode *netNode, struct sockaddr_in nextAddr, uint8_t minS, uint8_t maxS);
static void writeNetJoinMessage(unsigned char *destMessage, struct sockaddr_in addr, unsigned char range);
static int writeLookupResponse(unsigned char *destMessage, unsigned char *ssn, unsigned char *name, unsigned char *email, struct NetNode *netNode);
static void writeValInsertMessage(unsigned char *message, const char *ssn, const char *name, const char *email);
static void writeNetLeavingMessage(unsigned char *message, struct sockaddr_in addr);
static void writeArgvMessage(unsigned char *message, char *addr, char *port);
static struct STUN_RESPONSE_PDU readStunResponse(unsigned char *message);
static struct NET_JOIN_PDU readNetJoinMessage(unsigned char *message);
static struct NET_JOIN_RESPONSE_PDU readNetJoinResponse(unsigned char *message);
static struct NET_GET_NODE_RESPONSE_PDU readNetGetNodeResponse(unsigned char *message);
static struct VAL_INSERT_PDU *readValInsertMessage(unsigned char *message);
static struct VAL_LOOKUP_PDU readLookupMessage(unsigned char *message);
static struct NET_NEW_RANGE_PDU readNewRange(unsigned char *message);
static struct NET_LEAVING_PDU readNetLeavingMessage(unsigned char *message);
static void transferUpperRange(struct NetNode *netNode, uint8_t minS, uint8_t maxS);
static uint32_t deserializeUint32(unsigned char *message);
static uint16_t deserializeUint16(unsigned char *message);
static void serializeUint16(unsigned char *message, uint16_t value);
static void serializeUint32(unsigned char *message, uint32_t value);
static void removeMsgFromBuffer(unsigned char *buffer, int size);
static void printAddress(struct sockaddr_in addr);

// --------- DEBUG FUNCTIONS ----------- //
// static void fprintNetJoinResponse(unsigned char *response);
// static void fprint_address(struct NetNode *netNode, int socket);
// static void fprint_all_sockets(struct NetNode *netNode);
// static void printByteArray(unsigned char *buffer, int size);

static afEventHandler stateMachine = {
	[firstState] = {[eventDone] = gotoStateQ1},
	[q1] = {[eventDone] = gotoStateQ2},
	[q2] = {[eventStunResponse] = gotoStateQ3},
	[q3] = {[eventNodeResponse] = gotoStateQ7, [eventNodeResponseEmpty] = gotoStateQ4},
	[q4] = {[eventDone] = gotoStateQ6},
	[q5] = {[eventDone] = gotoStateQ6},
	[q6] = {[eventInsert] = gotoStateQ9, [eventLookup] = gotoStateQ9, [eventRemove] = gotoStateQ9, [eventShutDown] = gotoStateQ10, [eventJoin] = gotoStateQ12, [eventNewRange] = gotoStateQ15, [eventLeaving] = gotoStateQ16, [eventCloseConnection] = gotoStateQ17, [eventTimeout] = gotoStateQ6},
	[q7] = {[eventJoinResponse] = gotoStateQ8},
	[q8] = {[eventDone] = gotoStateQ6},
	[q9] = {[eventDone] = gotoStateQ6},
	[q10] = {[eventConnected] = gotoStateQ11, [eventNotConnected] = exitState},
	[q11] = {[eventNewRangeResponse] = gotoStateQ18},
	[q12] = {[eventNotConnected] = gotoStateQ5, [eventMaxNode] = gotoStateQ13, [eventNotMaxNode] = gotoStateQ14},
	[q18] = {[eventDone] = exitState},
	[q13] = {[eventDone] = gotoStateQ6},
	[q14] = {[eventDone] = gotoStateQ6},
	[q15] = {[eventDone] = gotoStateQ6},
	[q16] = {[eventDone] = gotoStateQ6},
	[q17] = {[eventDone] = gotoStateQ6}};

int main(int argc, char **argv)
{
	eSystemState nextState = firstState;
	eSystemEvent newEvent;

	check_params(argc);
	signal(SIGINT, sig_handler);

	struct NetNode netNode = {};
	memset(&netNode, 0, sizeof(netNode));

	netNode.pduMessage = calloc(BUFF_SIZE, sizeof(unsigned char));
	if (netNode.pduMessage == NULL)
	{
		exit_on_error("Calloc error", &netNode);
	}

	writeArgvMessage(netNode.pduMessage, argv[1], argv[2]);

	while (true)
	{
		newEvent = readEvent(&netNode, nextState);

		if ((nextState < lastState) && (newEvent < lastEvent) && stateMachine[nextState][newEvent] != NULL)
		{
			nextState = (*stateMachine[nextState][newEvent])(&netNode);
			if (nextState != q6)
			{
				if (nextState == lastState) {break;}
				printf("[Q%d]\n", nextState);
			}
		}
		else
		{
			if (nextState == lastState || newEvent == eventShutDown) {break;}
			fprintf(stderr, "Invalid state Q%d or event E%d\n", nextState, newEvent);
			break;
		}
	}

	return 0;
}

static void check_params(int argc)
{
	if (argc != 3)
	{
		exit_on_error_custom("Usage: ", " node <Tracker Address> <Tracker Port>");
	}
}

static void exit_on_error(const char *title, struct NetNode *netNode)
{
	if (errno != EINTR)
	{
		exitState(netNode);
		perror(title);
		exit(1);
	}
}

static void exit_on_error_custom(const char *title, const char *detail)
{
	if (errno != EINTR)
	{
		fprintf(stderr, "%s:%s\n", title, detail);
		exit(1);
	}
}

static eSystemEvent readEvent(struct NetNode *netNode, eSystemState state)
{
	switch (state)
	{
	case firstState:
	case q1:
	case q4:
	case q8:
	case q5:
	case q9:
	case q13:
	case q14:
	case q15:
	case q16:
	case q17:
		return eventDone;
	case q18:
		return eventShutDown;
	case q12:
		if (!nodeConnected(netNode))
		{ //node not connected
			return eventNotConnected;
		}
		else
		{ //node = max_node
			if ((netNode->nodeRange.max - netNode->nodeRange.min) == netNode->pduMessage[7])
			{
				return eventMaxNode;
			}
			else //node != max_node
			{
				return eventNotMaxNode;
			}
		}
	case q10:
		if (nodeConnected(netNode))
		{
			return eventConnected;
		}
		else
		{
			printf("\tI am the last node, bye!\n");
			return eventNotConnected;
		}
	default:
		if (netNode->pduMessage[0] != 0)
		{ 	
			//Messages left in buffer
			return findRightEvent(netNode, netNode->pduMessage, BUFF_SIZE);
		}
		else
		{
			return readFromSockets(netNode);
		}
	}
}

static eSystemEvent readFromSockets(struct NetNode *netNode)
{
	int timeoutMs = 5000;
	int timeoutCount = 0;
	int returnValue;
	unsigned char buffer[BUFF_SIZE];
	memset(buffer, 0, BUFF_SIZE);
	ssize_t bytesRead = 0;

	netNode->fds[UDP_SOCKET_A].events = POLLIN;
	netNode->fds[TCP_SOCKET_B].events = POLLIN;
	netNode->fds[TCP_SOCKET_C].events = POLLIN;
	netNode->fds[TCP_SOCKET_D].events = POLLIN;
	netNode->fds[UDP_SOCKET_A2].events = POLLIN;

	returnValue = poll(netNode->fds, NO_SOCKETS, timeoutMs);

	if (returnValue == -1)
	{
		exit_on_error("Poll error", netNode);
	}
	else if (errno == EINTR)
	{
		errno = 0;
		return eventShutDown;
	}

	if (returnValue > 0)
	{
		for (int i = 0; i < NO_SOCKETS; i++)
		{
			if (netNode->fds[i].revents & POLLIN)
			{
				bytesRead = read(netNode->fds[i].fd, buffer, BUFF_SIZE);
				if (bytesRead > 0)
				{
					//Copy buffer to message buffer
					memcpy(netNode->pduMessage, buffer, bytesRead);
					return findRightEvent(netNode, buffer, bytesRead);
				}
			}
		}
		return findRightEvent(netNode, buffer, bytesRead);
	}
	else
	{
		if (timeoutCount % 3 == 0)
		{
			return eventTimeout;
		}
		else
		{
			printf("[Q6] (%d entries stored)\n", list_get_length(netNode->entries));
		}
		timeoutCount++;
	}

	return lastEvent;
}

static eSystemEvent findRightEvent(struct NetNode *netNode, unsigned char *buffer, ssize_t buffSize)
{
	struct NET_GET_NODE_RESPONSE_PDU getNodeResponse;

	switch (buffer[0])
	{
	case STUN_RESPONSE:
		return eventStunResponse;
	case NET_GET_NODE_RESPONSE:
		getNodeResponse = readNetGetNodeResponse(netNode->pduMessage);
		return (getNodeResponse.address == 0 && getNodeResponse.port == 0) ? eventNodeResponseEmpty : eventNodeResponse;
	case NET_JOIN_RESPONSE:
		return eventJoinResponse;
	case VAL_INSERT:
		return eventInsert;
	case VAL_LOOKUP:
		return eventLookup;
	case VAL_REMOVE:
		return eventRemove;
	case NET_JOIN:
		return eventJoin;
	case NET_NEW_RANGE:
		return eventNewRange;
	case NET_LEAVING:
		return eventLeaving;
	case NET_CLOSE_CONNECTION:
		return eventCloseConnection;
	case NET_NEW_RANGE_RESPONSE:
		return eventNewRangeResponse;
	default:
		fprintf(stderr, "Unknown response: %d\n", buffer[0]);
		return lastEvent;
	}
}

eSystemState gotoStateQ1(struct NetNode *netNode)
{
	//Read arguments (address and port)
	int addrLen = strlen((char *)netNode->pduMessage);
	char addr[addrLen + 1];
	memcpy(addr, netNode->pduMessage, addrLen);
	addr[addrLen] = '\0';

	int portLen = strlen((char *)&netNode->pduMessage[addrLen + 1]);
	char port[portLen + 1];
	memcpy(port, &netNode->pduMessage[addrLen + 1], portLen);
	port[portLen] = '\0';

	//Set up udp socket to tracker
	netNode->fds[UDP_SOCKET_A].fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (netNode->fds[UDP_SOCKET_A].fd == -1)
	{
		exit_on_error("Could not open socket to tracker", netNode);
	}

	//Set address to tracker
	netNode->fdsAddr[UDP_SOCKET_A].sin_family = AF_INET;
	netNode->fdsAddr[UDP_SOCKET_A].sin_port = htons(strtol(port, NULL, 10));
	if (inet_aton(addr, &netNode->fdsAddr[UDP_SOCKET_A].sin_addr) == 0)
	{
		exit_on_error_custom("Could not convert address with inet_aton", addr);
	}

	//Send STUN_LOOKUP
	unsigned char lookupMessage[1] = {'\0'};
	size_t lookupSize = sizeof(lookupMessage);
	lookupMessage[0] = STUN_LOOKUP;
	socklen_t UDPAddressLen = sizeof(netNode->fdsAddr[UDP_SOCKET_A]);

	if (sendto(netNode->fds[UDP_SOCKET_A].fd, lookupMessage, lookupSize, 0, (struct sockaddr *)&netNode->fdsAddr[UDP_SOCKET_A], UDPAddressLen) == -1)
	{
		exit_on_error("Could not send to tracker with UDP", netNode);
	}

	removeMsgFromBuffer(netNode->pduMessage, 1 + addrLen + portLen);
	return q1;
}

eSystemState gotoStateQ2(struct NetNode *netNode)
{
	printf("\tNode started, sending STUN_LOOKUP to tacker: V4(");
	printAddress(netNode->fdsAddr[UDP_SOCKET_A]);
	printf(")\n");

	return q2;
}

eSystemState gotoStateQ3(struct NetNode *netNode)
{
	struct STUN_RESPONSE_PDU stunResponse = readStunResponse(netNode->pduMessage);
	struct sockaddr_in myAddr;
	myAddr.sin_addr.s_addr = htonl(stunResponse.address);
	printf("\tGot STUN_RESPONSE, my address is: %s\n", inet_ntoa(myAddr.sin_addr));

	//Send NET_GET_NODE
	unsigned char getNodeMessage[1] = {'\0'};
	size_t messageSize = sizeof(getNodeMessage);
	getNodeMessage[0] = NET_GET_NODE;
	socklen_t addrLength = sizeof(netNode->fdsAddr[UDP_SOCKET_A]);

	if (sendto(netNode->fds[UDP_SOCKET_A].fd, getNodeMessage, messageSize, 0, (struct sockaddr *)&netNode->fdsAddr[UDP_SOCKET_A], addrLength) == -1)
	{
		exit_on_error("Could not send to Tracker with UDP", netNode);
	}

	removeMsgFromBuffer(netNode->pduMessage, STUN_RESP_SIZE);
	return q3;
}

eSystemState gotoStateQ4(struct NetNode *netNode)
{
	netNode->nodeRange.min = 0;
	netNode->nodeRange.max = 255;
	netNode->entries = list_create();

	printf("\tI am the first node to join the network\n");

	removeMsgFromBuffer(netNode->pduMessage, GET_NODE_RESP_SIZE);
	return q4;
}

eSystemState gotoStateQ5(struct NetNode *netNode)
{
	//Node not connected, connect to successor
	//Connect to src address in message and store connection as succ address
	struct NET_JOIN_PDU joinRequest = readNetJoinMessage(netNode->pduMessage);

	netNode->fdsAddr[TCP_SOCKET_B].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr = htonl(joinRequest.src_address);
	netNode->fdsAddr[TCP_SOCKET_B].sin_port = htons(joinRequest.src_port);
	socklen_t addrLenB = sizeof(netNode->fdsAddr[TCP_SOCKET_B]);

	netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_B].fd == -1)
	{
		exit_on_error("Could not open socket to successor", netNode);
	}

	if (connect(netNode->fds[TCP_SOCKET_B].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_B], addrLenB) == -1)
	{
		exit_on_error("Could not connect to successor", netNode);
	}

	printf("\tConnected to new successor V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_B]);

	// Open socket
	if (netNode->fds[TCP_SOCKET_C].fd == 0) //Socket C not initialized yet
	{
		initTCPSocketC(netNode);
	}

	unsigned char minP = netNode->nodeRange.min;
	unsigned char maxP = netNode->nodeRange.max;
	unsigned char maxS = maxP;
	maxP = (unsigned char)((maxP - minP) / 2) + minP;
	unsigned char minS = maxP + 1;

	netNode->nodeRange.min = minP;
	netNode->nodeRange.max = maxP;

	printf("\tOther hash-range is (%d, %d)\n", minS, maxS);
	printf("\tNew hash-range is (%d, %d)\n", netNode->nodeRange.min, netNode->nodeRange.max);

	//Send NET_JOIN_RESPONSE
	unsigned char netJoinResponseMessage[9] = {'\0'};
	size_t messageSize = sizeof(netJoinResponseMessage);
	writeNetJoinResponse(netJoinResponseMessage, netNode, netNode->fdsAddr[TCP_SOCKET_C], minS, maxS);

	if (send(netNode->fds[TCP_SOCKET_B].fd, netJoinResponseMessage, messageSize, 0) == -1)
	{
		exit_on_error("Could not send to successor", netNode);
	}

	// Accept predecessor
	if (listen(netNode->fds[TCP_SOCKET_C].fd, 1) == -1)
	{
		exit_on_error("Did not hear anything from my connection socket", netNode);
	}

	socklen_t addrLenD = sizeof(netNode->fdsAddr[TCP_SOCKET_D]);
	netNode->fds[TCP_SOCKET_D].fd = accept(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_D], &addrLenD);
	if (netNode->fds[TCP_SOCKET_D].fd == -1)
	{
		exit_on_error("Could not accept from my predecessor", netNode);
	}

	//Transfer upper half of entry-range to successor
	transferUpperRange(netNode, minS, maxS);

	printf("\tAccept new predecessor V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_D]);
	printf(")\n");

	removeMsgFromBuffer(netNode->pduMessage, JOIN_SIZE);
	return q5;
}

eSystemState gotoStateQ6(struct NetNode *netNode)
{
	printf("[Q6] (%d entries stored) (%d, %d)\n", list_get_length(netNode->entries), netNode->nodeRange.min, netNode->nodeRange.max);

	//Send NET_ALIVE
	unsigned char netAliveMessage[1] = {'\0'};
	size_t messageSize = sizeof(netAliveMessage);
	socklen_t addrLength = sizeof(netNode->fdsAddr[UDP_SOCKET_A]);
	netAliveMessage[0] = NET_ALIVE;

	if (sendto(netNode->fds[UDP_SOCKET_A].fd, netAliveMessage, messageSize, 0, (struct sockaddr *)&netNode->fdsAddr[UDP_SOCKET_A], addrLength) == -1)
	{
		exit_on_error("Could not send NET_ALIVE to tracker", netNode);
	}

	return q6;
}

eSystemState gotoStateQ7(struct NetNode *netNode)
{
	netNode->fds[UDP_SOCKET_A2].fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (netNode->fds[UDP_SOCKET_A2].fd == -1)
	{
		exit_on_error("Could not open the second UDP socket", netNode);
	}

	//Init UDP A2 address
	struct NET_GET_NODE_RESPONSE_PDU getNodeResponse = readNetGetNodeResponse(netNode->pduMessage);
	netNode->fdsAddr[UDP_SOCKET_A2].sin_family = AF_INET;
	netNode->fdsAddr[UDP_SOCKET_A2].sin_addr.s_addr = htonl(getNodeResponse.address);
	netNode->fdsAddr[UDP_SOCKET_A2].sin_port = htons(getNodeResponse.port);

	initTCPSocketC(netNode);

	//Send NET_JOIN
	unsigned char netJoinMessage[JOIN_SIZE] = {'\0'};
	size_t messageSize = sizeof(netJoinMessage);
	writeNetJoinMessage(netJoinMessage, netNode->fdsAddr[TCP_SOCKET_C], 0);
	socklen_t udpAddressLen = sizeof(netNode->fdsAddr[UDP_SOCKET_A2]);

	printf("\tI am not the first node, sending NET_JOIN to V4(");
	printAddress(netNode->fdsAddr[UDP_SOCKET_A2]);
	printf(")\n");

	if (sendto(netNode->fds[UDP_SOCKET_A2].fd, netJoinMessage, messageSize, 0, (struct sockaddr *)&netNode->fdsAddr[UDP_SOCKET_A2], udpAddressLen) == -1)
	{
		exit_on_error("Could not send to second UDP connection", netNode);
	}

	if (listen(netNode->fds[TCP_SOCKET_C].fd, 1) == -1)
	{
		exit_on_error("Could not hear anything from my open TCP", netNode);
	}

	socklen_t preNodeLen = sizeof(netNode->fdsAddr[TCP_SOCKET_D]);
	netNode->fds[TCP_SOCKET_D].fd = accept(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_D], &preNodeLen);
	if (netNode->fds[TCP_SOCKET_D].fd == -1)
	{
		exit_on_error("Could not accept from predecessor", netNode);
	}

	printf("\tAccepted new predecessor V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_D]);
	printf(")\n");

	removeMsgFromBuffer(netNode->pduMessage, NET_GET_NODE_RESPONSE);
	return q7;
}

eSystemState gotoStateQ8(struct NetNode *netNode)
{
	netNode->entries = list_create();

	netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_B].fd == -1)
	{
		exit_on_error("Could not open socket to successor", netNode);
	}

	struct NET_JOIN_RESPONSE_PDU joinResponse = readNetJoinResponse(netNode->pduMessage);
	netNode->fdsAddr[TCP_SOCKET_B].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr = htonl(joinResponse.next_address);
	netNode->fdsAddr[TCP_SOCKET_B].sin_port = htons(joinResponse.next_port);
	socklen_t addrLenB = sizeof(netNode->fdsAddr[TCP_SOCKET_B]);

	netNode->nodeRange.min = joinResponse.range_start;
	netNode->nodeRange.max = joinResponse.range_end;

	printf("\tGot NET_JOIN_RESPONSE from V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_D]);
	printf("), my range is (%d, %d)\n", netNode->nodeRange.min, netNode->nodeRange.max);
	printf("\tConnecting to successor V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_B]);
	printf(")\n");
	printf("\tConnecting to new successor V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_B]);
	printf(")\n");

	if (connect(netNode->fds[TCP_SOCKET_B].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_B], addrLenB) == -1)
	{
		exit_on_error("Could not connect to successor", netNode);
	}

	removeMsgFromBuffer(netNode->pduMessage, JOIN_RESP_SIZE);
	return q8;
}

eSystemState gotoStateQ9(struct NetNode *netNode)
{
	struct VAL_INSERT_PDU *insertMessage;
	int messageSize = 0;
	unsigned char ssn[SSN_LENGTH + 1] = {'\0'};
	memcpy(ssn, &netNode->pduMessage[1], SSN_LENGTH);
	hash_t hash = hash_ssn((char *)ssn);

	if (hash >= netNode->nodeRange.min && hash <= netNode->nodeRange.max)
	{ //If HASH(entry) is in node -> store/respond/delete
		if (netNode->pduMessage[0] == VAL_INSERT)
		{
			insertMessage = readValInsertMessage(netNode->pduMessage);

			char name[insertMessage->name_length + 1];
			char email[insertMessage->email_length + 1];

			name[insertMessage->name_length] = '\0';
			email[insertMessage->email_length] = '\0';

			memcpy(name, insertMessage->name, insertMessage->name_length);
			memcpy(email, insertMessage->email, insertMessage->email_length);

			messageSize = 3 + SSN_LENGTH + insertMessage->name_length + insertMessage->email_length;

			free(insertMessage->name);
			free(insertMessage->email);
			free(insertMessage);

			list_insert(list_first(netNode->entries), (char *)ssn, email, name);
			printf("\tInserting ssn Entry { ssn: \"%s\", name: \"%s\", email: \"%s\" }\n", ssn, name, email);
		}
		else if (netNode->pduMessage[0] == VAL_REMOVE)
		{
			//Remove ssn if found
			if (!list_is_empty(netNode->entries))
			{
				ListPos pos = list_first(netNode->entries);
				while (!list_pos_equal(pos, list_end(netNode->entries)))
				{ //Find ssn
					const char *listSSN = list_inspect_ssn(pos);
					if (strncmp((char *)ssn, (char *)listSSN, SSN_LENGTH) == 0)
					{
						//Remove index found
						list_remove(pos);
						printf("Removing ssn %s\n", ssn);
						break;
					}
					pos = list_next(pos);
				}
			}
			messageSize = REMOVE_SIZE;
		}
		else
		{ //Do lookup
			struct VAL_LOOKUP_PDU lookupMessage = readLookupMessage(netNode->pduMessage);
			int bytesWritten = 0;
			unsigned char lookupResponse[BUFF_SIZE];

			if (!list_is_empty(netNode->entries))
			{
				ListPos pos = list_first(netNode->entries);

				while (!list_pos_equal(pos, list_end(netNode->entries)))
				{ //Find ssn
					const char *listSSN = list_inspect_ssn(pos);
					if (strncmp((char *)ssn, listSSN, SSN_LENGTH) == 0)
					{
						const char *name = list_inspect_name(pos);
						const char *email = list_inspect_email(pos);
						bytesWritten = writeLookupResponse(lookupResponse, ssn, (unsigned char *)name, (unsigned char *)email, netNode);

						break;
					}
					pos = list_next(pos);
				}
			}

			if (bytesWritten > 0)
			{ //Send response
				struct sockaddr_in senderAddr;
				senderAddr.sin_family = AF_INET;
				senderAddr.sin_addr.s_addr = htonl(lookupMessage.sender_address);
				senderAddr.sin_port = htons(lookupMessage.sender_port);
				socklen_t addrLen = sizeof(senderAddr);

				if (sendto(netNode->fds[UDP_SOCKET_A].fd, lookupResponse, bytesWritten, 0, (struct sockaddr *)&senderAddr, addrLen) == -1)
				{
					exit_on_error("Could not send response to tracker", netNode);
				}
			}
			messageSize = LOOKUP_SIZE;
		}
	}
	else
	{ //HASH(entry) NOT in node, forward message
		char *choice;
		if (netNode->pduMessage[0] == VAL_INSERT)
		{
			insertMessage = readValInsertMessage(netNode->pduMessage);
			messageSize = 3 + SSN_LENGTH + insertMessage->name_length + insertMessage->email_length;

			free(insertMessage->name);
			free(insertMessage->email);
			free(insertMessage);

			choice = "val_insert";
		}
		else if (netNode->pduMessage[0] == VAL_REMOVE)
		{
			messageSize = REMOVE_SIZE;
			choice = "val_remove";
		}
		else
		{ //VAL_LOOKUP
			messageSize = LOOKUP_SIZE;
			choice = "val_lookup";
		}

		printf("\tForwarding %s to successor\n", choice);
		if (send(netNode->fds[TCP_SOCKET_B].fd, netNode->pduMessage, messageSize, 0) == -1)
		{
			exit_on_error("Could not forward NET_INSERT to successor", netNode);
		}
	}

	removeMsgFromBuffer(netNode->pduMessage, messageSize);
	return q9;
}

eSystemState gotoStateQ10(struct NetNode *netNode)
{
	return q10;
}

eSystemState gotoStateQ11(struct NetNode *netNode)
{
	unsigned char newRangeMessage[3] = {'\0'};
	newRangeMessage[0] = NET_NEW_RANGE;
	newRangeMessage[1] = netNode->nodeRange.min;
	newRangeMessage[2] = netNode->nodeRange.max;
	size_t messageSize = sizeof(newRangeMessage);

	if (netNode->nodeRange.min == 0)
	{
		if (send(netNode->fds[TCP_SOCKET_B].fd, newRangeMessage, messageSize, 0) == -1)
		{
			perror("Could not send to successor");
			exit(1);
		}
	}
	else
	{
		if (send(netNode->fds[TCP_SOCKET_D].fd, newRangeMessage, messageSize, 0) == -1)
		{
			perror("Could not send to successor");
			exit(1);
		}
	}

	return q11;
}

eSystemState gotoStateQ12(struct NetNode *netNode)
{
	struct NET_JOIN_PDU joinRequest = readNetJoinMessage(netNode->pduMessage);

	//Update PDU max fields
	unsigned char range = netNode->nodeRange.max - netNode->nodeRange.min;
	if (range > joinRequest.max_span)
	{
		printf("\tI am the node with the maximum span! (%d)\n", range);
		netNode->pduMessage[7] = range;
		serializeUint32(&netNode->pduMessage[8], netNode->fdsAddr[TCP_SOCKET_C].sin_addr.s_addr);
		serializeUint16(&netNode->pduMessage[12], netNode->fdsAddr[TCP_SOCKET_C].sin_port);
	}

	return q12;
}

eSystemState gotoStateQ13(struct NetNode *netNode)
{
	//Store address to old successor
	struct sockaddr_in oldSuccessor;
	oldSuccessor.sin_family = AF_INET;
	oldSuccessor.sin_addr.s_addr = netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr;
	oldSuccessor.sin_port = netNode->fdsAddr[TCP_SOCKET_B].sin_port;

	//Send NET_CLOSE_CONNECTION to successor
	unsigned char closeConnectionMessage[1] = {'\0'};
	size_t messageCloseSize = sizeof(closeConnectionMessage);
	closeConnectionMessage[0] = NET_CLOSE_CONNECTION;

	printf("\tSending NET_CLOSE_CONNECTION to successor\n");
	if (send(netNode->fds[TCP_SOCKET_B].fd, closeConnectionMessage, messageCloseSize, 0) == -1)
	{
		exit_on_error("Could not send NET_CLOSE_CONNECTION to successor", netNode);
	}

	//Close and reopen socket B
	close(netNode->fds[TCP_SOCKET_B].fd);
	netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_B].fd == -1)
	{
		exit_on_error("Could not connect to new successor", netNode);
	}

	//Connect to prospect
	struct NET_JOIN_PDU joinRequest = readNetJoinMessage(netNode->pduMessage);
	netNode->fdsAddr[TCP_SOCKET_B].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr = htonl(joinRequest.src_address);
	netNode->fdsAddr[TCP_SOCKET_B].sin_port = htons(joinRequest.src_port);
	socklen_t addLenB = sizeof(netNode->fdsAddr[TCP_SOCKET_B]);

	if (connect(netNode->fds[TCP_SOCKET_B].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_B], addLenB) == -1)
	{
		exit_on_error("Could not connect to new successor", netNode);
	}

	printf("\tConnected to new successor V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_B]);
	printf(")\n");
	printf("\tConnected to new successor Ok(V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_B]);
	printf("))\n");

	unsigned char minP = netNode->nodeRange.min;
	unsigned char maxP = netNode->nodeRange.max;
	unsigned char maxS = maxP;
	maxP = (unsigned char)((maxP - minP) / 2) + minP;
	unsigned char minS = maxP + 1;

	netNode->nodeRange.min = minP;
	netNode->nodeRange.max = maxP;

	//Send NET_JOIN_RESPONSE to prospect
	unsigned char netJoinResponseMessage[9] = {'\0'};
	size_t messageJoinSize = sizeof(netJoinResponseMessage);
	writeNetJoinResponse(netJoinResponseMessage, netNode, oldSuccessor, minS, maxS);

	printf("\tOther hash-range is (%d,%d)\n", minS, maxS);
	printf("\tNew hash-range is (%d,%d)\n", minP, maxP);
	printf("\tSending join response\n");

	if (send(netNode->fds[TCP_SOCKET_B].fd, netJoinResponseMessage, messageJoinSize, 0) == -1)
	{
		exit_on_error("Could not send NET_JOIN", netNode);
	}
	transferUpperRange(netNode, minS, maxS);

	removeMsgFromBuffer(netNode->pduMessage, JOIN_SIZE);
	return q13;
}

eSystemState gotoStateQ14(struct NetNode *netNode)
{
	printf("\tForwarding to successor\n");

	if (send(netNode->fds[TCP_SOCKET_B].fd, netNode->pduMessage, JOIN_SIZE, 0) == -1)
	{
		exit_on_error("Could not forward message to successor", netNode);
	}

	removeMsgFromBuffer(netNode->pduMessage, JOIN_SIZE);
	return q14;
}

eSystemState gotoStateQ15(struct NetNode *netNode)
{
	struct NET_NEW_RANGE_PDU newRange = readNewRange(netNode->pduMessage);
	unsigned char newRangeResponse[1] = {'\0'};
	newRangeResponse[0] = NET_NEW_RANGE_RESPONSE;
	size_t messageSize = sizeof(newRangeResponse);

	printf("\tCurrent range is: (%d, %d)\n", netNode->nodeRange.min, netNode->nodeRange.max);

	if (newRange.range_start < netNode->nodeRange.min)
	{
		printf("\tSending NET_NEW_RANGE_RESPONSE to predecessor\n");

		netNode->nodeRange.min = newRange.range_start;
		if (send(netNode->fds[TCP_SOCKET_D].fd, newRangeResponse, messageSize, 0) == -1)
		{
			exit_on_error("Could not send NET_NEW_RANGE_RESPONSE to predecessor", netNode);
		}
	}
	else
	{
		printf("\tSending NET_NEW_RANGE_RESPONSE to successor\n");

		netNode->nodeRange.max = newRange.range_end;
		if (send(netNode->fds[TCP_SOCKET_B].fd, newRangeResponse, messageSize, 0) == -1)
		{
			exit_on_error("Could not send NET_NEW_RANGE_RESPONSE to successor", netNode);
		}
	}
	printf("\tNew range is: (%d, %d)\n", netNode->nodeRange.min, netNode->nodeRange.max);

	removeMsgFromBuffer(netNode->pduMessage, NEW_RANGE_SIZE);
	return q15;
}

eSystemState gotoStateQ16(struct NetNode *netNode)
{
	printf("\tRemote closed the connection Ok(V4(");
	printAddress(netNode->fdsAddr[TCP_SOCKET_B]);
	printf("))\n");

	//Close and reopen socket to successor
	shutdown(netNode->fds[TCP_SOCKET_B].fd, SHUT_WR);
	close(netNode->fds[TCP_SOCKET_B].fd);
	netNode->fds[TCP_SOCKET_B].fd = 0;

	struct NET_LEAVING_PDU leavingMessage = readNetLeavingMessage(netNode->pduMessage);
	netNode->fdsAddr[TCP_SOCKET_B].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_B].sin_addr.s_addr = htonl(leavingMessage.new_address);
	netNode->fdsAddr[TCP_SOCKET_B].sin_port = htons(leavingMessage.new_port);
	socklen_t addrLenB = sizeof(netNode->fdsAddr[TCP_SOCKET_B]);

	if (netNode->nodeRange.min == 0 && netNode->nodeRange.max == 255)
	{
		printf("\tI am the last node\n");
	}
	else
	{
		netNode->fds[TCP_SOCKET_B].fd = socket(AF_INET, SOCK_STREAM, 0);
		if (netNode->fds[TCP_SOCKET_B].fd == -1)
		{
			exit_on_error("Could not open socket to successor", netNode);
		}
		if (connect(netNode->fds[TCP_SOCKET_B].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_B], addrLenB) == -1)
		{
			exit_on_error("Could not connect to successor", netNode);
		}

		printf("\tConnected to new successor V4(");
		printAddress(netNode->fdsAddr[TCP_SOCKET_B]);
		printf(")\n");
	}

	removeMsgFromBuffer(netNode->pduMessage, LEAVING_SIZE);
	return q16;
}

eSystemState gotoStateQ17(struct NetNode *netNode)
{
	printf("\tDisconnecting from predecessor\n");

	shutdown(netNode->fds[TCP_SOCKET_D].fd, SHUT_WR);
	close(netNode->fds[TCP_SOCKET_D].fd);
	netNode->fds[TCP_SOCKET_D].fd = 0;

	if (!(netNode->nodeRange.min == 0 && netNode->nodeRange.max == 255))
	{
		//If predecessor is not successor
		netNode->fds[TCP_SOCKET_D].fd = socket(AF_INET, SOCK_STREAM, 0);
		if (netNode->fds[TCP_SOCKET_D].fd == -1)
		{
			exit_on_error("Could not open socket to predecessor", netNode);
		}
		printf("\tAwaiting new predecessor\n");

		socklen_t addrLenD = sizeof(TCP_SOCKET_D);
		netNode->fds[TCP_SOCKET_D].fd = accept(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_D], &addrLenD);

		printf("\tAccepted new predecessor V4(");
		printAddress(netNode->fdsAddr[TCP_SOCKET_D]);
		printf(")\n");
	}
	else
	{
		printf("\tI am the last node\n");
	}

	removeMsgFromBuffer(netNode->pduMessage, CLOSE_CON_SIZE);
	return q17;
}

eSystemState gotoStateQ18(struct NetNode *netNode)
{
	if (!list_is_empty(netNode->entries))
	{
		ListPos pos = list_first(netNode->entries);
		while (!list_pos_equal(pos, list_end(netNode->entries)))
		{
			const char *ssn = list_inspect_ssn(pos);
			const char *name = list_inspect_name(pos);
			const char *email = list_inspect_email(pos);

			unsigned char nameLen = strlen(name);
			unsigned char emailLen = strlen(email);

			size_t messageSize = 3 + SSN_LENGTH + nameLen + emailLen;
			unsigned char insertMessage[messageSize];
			writeValInsertMessage(insertMessage, ssn, name, email);

			if (send(netNode->fds[TCP_SOCKET_B].fd, insertMessage, messageSize, 0) == -1)
			{
				perror("Could not send to successor");
				exit(1);
			}
			pos = list_next(pos);
		}
	}
	list_destroy(netNode->entries);
	netNode->entries = NULL;

	unsigned char closeMessage[1] = {'\0'};
	closeMessage[0] = NET_CLOSE_CONNECTION;
	size_t messageSize = sizeof(closeMessage);

	if (send(netNode->fds[TCP_SOCKET_B].fd, closeMessage, messageSize, 0) == -1)
	{
		perror("Could not send to successor");
		exit(1);
	}

	unsigned char leavingMessage[LEAVING_SIZE] = {'\0'};
	messageSize = sizeof(leavingMessage);
	writeNetLeavingMessage(leavingMessage, netNode->fdsAddr[TCP_SOCKET_B]);

	if (send(netNode->fds[TCP_SOCKET_D].fd, leavingMessage, messageSize, 0) == -1)
	{
		perror("Could not send to predecessor");
		exit(1);
	}
	printf("\tTransferring all entries to successor\n");
	printf("\tSending NET_LEAVING to predecessor\n");

	removeMsgFromBuffer(netNode->pduMessage, NEW_RANGE_RES_SIZE);
	return q18;
}

eSystemState exitState(struct NetNode *netNode)
{
	shutdown(netNode->fds[UDP_SOCKET_A].fd, SHUT_WR);
	close(netNode->fds[UDP_SOCKET_A].fd);

	shutdown(netNode->fds[TCP_SOCKET_B].fd, SHUT_WR);
	close(netNode->fds[TCP_SOCKET_B].fd);

	shutdown(netNode->fds[TCP_SOCKET_C].fd, SHUT_WR);
	close(netNode->fds[TCP_SOCKET_C].fd);

	shutdown(netNode->fds[TCP_SOCKET_D].fd, SHUT_WR);
	close(netNode->fds[TCP_SOCKET_D].fd);

	shutdown(netNode->fds[UDP_SOCKET_A2].fd, SHUT_WR);
	close(netNode->fds[UDP_SOCKET_A2].fd);

	if (netNode->entries)
	{
		list_destroy(netNode->entries);
	}
	if (netNode->pduMessage)
	{
		free(netNode->pduMessage);
	}

	return lastState;
}

// If socket D or B has been opened the node is connected
static bool nodeConnected(struct NetNode *netNode)
{
	return netNode->fds[TCP_SOCKET_D].fd != 0 || netNode->fds[TCP_SOCKET_B].fd != 0;
}

static void initTCPSocketC(struct NetNode *netNode)
{
	netNode->fds[TCP_SOCKET_C].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (netNode->fds[TCP_SOCKET_C].fd == -1)
	{
		exit_on_error("socket error", netNode);
	}

	memset(&netNode->fdsAddr[TCP_SOCKET_C], 0, sizeof(netNode->fdsAddr[TCP_SOCKET_C]));
	netNode->fdsAddr[TCP_SOCKET_C].sin_family = AF_INET;
	netNode->fdsAddr[TCP_SOCKET_C].sin_addr.s_addr = INADDR_ANY;
	netNode->fdsAddr[TCP_SOCKET_C].sin_port = 0;
	socklen_t addrLenC = sizeof(netNode->fdsAddr[TCP_SOCKET_C]);

	int reuseaddr = 1;
	if (setsockopt(netNode->fds[TCP_SOCKET_C].fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
	{
		exit_on_error("setsockopt error", netNode);
	}
	if (bind(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_C], addrLenC) == -1)
	{
		exit_on_error("Could not bind to open TCP socket", netNode);
	}
	if (getsockname(netNode->fds[TCP_SOCKET_C].fd, (struct sockaddr *)&netNode->fdsAddr[TCP_SOCKET_C], &addrLenC) == -1)
	{
		exit_on_error("getsockname error", netNode);
	}
}

static struct STUN_RESPONSE_PDU readStunResponse(unsigned char *message)
{
	static struct STUN_RESPONSE_PDU stunResponse;
	stunResponse.type = message[0];
	stunResponse.address = deserializeUint32(&message[1]);

	return stunResponse;
}

static struct NET_GET_NODE_RESPONSE_PDU readNetGetNodeResponse(unsigned char *message)
{
	struct NET_GET_NODE_RESPONSE_PDU getNodeResponse;
	getNodeResponse.type = message[0];
	getNodeResponse.address = deserializeUint32(&message[1]);
	getNodeResponse.port = deserializeUint16(&message[5]);

	return getNodeResponse;
}

static struct NET_JOIN_PDU readNetJoinMessage(unsigned char *message)
{
	struct NET_JOIN_PDU joinRequest;
	joinRequest.type = message[0];
	joinRequest.src_address = deserializeUint32(&message[1]);
	joinRequest.src_port = deserializeUint16(&message[5]);
	joinRequest.max_span = message[7];
	joinRequest.max_address = deserializeUint32(&message[8]);
	joinRequest.max_port = deserializeUint16(&message[12]);

	return joinRequest;
}

static struct NET_JOIN_RESPONSE_PDU readNetJoinResponse(unsigned char *message)
{
	struct NET_JOIN_RESPONSE_PDU joinResponse;
	joinResponse.type = message[0];
	joinResponse.next_address = deserializeUint32(&message[1]);
	joinResponse.next_port = deserializeUint16(&message[5]);
	joinResponse.range_start = message[7];
	joinResponse.range_end = message[8];

	return joinResponse;
}

static struct NET_NEW_RANGE_PDU readNewRange(unsigned char *message)
{
	struct NET_NEW_RANGE_PDU newRange;
	newRange.type = message[0];
	newRange.range_start = message[1];
	newRange.range_end = message[2];

	return newRange;
}

static struct VAL_INSERT_PDU *readValInsertMessage(unsigned char *message)
{
	struct VAL_INSERT_PDU *insertMessage = malloc(sizeof(struct VAL_INSERT_PDU));
	insertMessage->type = message[0];
	memcpy(insertMessage->ssn, &message[1], SSN_LENGTH);

	insertMessage->name_length = message[SSN_LENGTH + 1];
	insertMessage->name = malloc(sizeof(unsigned char) * insertMessage->name_length);
	memcpy(insertMessage->name, &message[SSN_LENGTH + 2], insertMessage->name_length);

	insertMessage->email_length = message[SSN_LENGTH + 2 + insertMessage->name_length];
	insertMessage->email = malloc(sizeof(unsigned char) * insertMessage->email_length);
	memcpy(insertMessage->email, &message[SSN_LENGTH + 3 + insertMessage->name_length], insertMessage->email_length);

	return insertMessage;
}

static struct VAL_LOOKUP_PDU readLookupMessage(unsigned char *message)
{
	struct VAL_LOOKUP_PDU lookupMessage;
	lookupMessage.type = message[0];
	memcpy(lookupMessage.ssn, &message[1], SSN_LENGTH);
	lookupMessage.sender_address = deserializeUint32(&message[13]);
	lookupMessage.sender_port = deserializeUint16(&message[17]);

	return lookupMessage;
}

static struct NET_LEAVING_PDU readNetLeavingMessage(unsigned char *message)
{
	struct NET_LEAVING_PDU leavingMessage;
	leavingMessage.type = message[0];
	leavingMessage.new_address = deserializeUint32(&message[1]);
	leavingMessage.new_port = deserializeUint16(&message[5]);

	return leavingMessage;
}

static void writeNetJoinMessage(unsigned char *destMessage, struct sockaddr_in addr, unsigned char range)
{
	destMessage[0] = NET_JOIN;
	serializeUint32(&destMessage[1], addr.sin_addr.s_addr);
	serializeUint16(&destMessage[5], addr.sin_port);
	destMessage[7] = range;
	serializeUint32(&destMessage[8], (uint32_t)0);
	serializeUint16(&destMessage[12], (uint16_t)0);
}

static void writeNetJoinResponse(unsigned char *destMessage, struct NetNode *netNode, struct sockaddr_in nextAddr, uint8_t minS, uint8_t maxS)
{

	destMessage[0] = NET_JOIN_RESPONSE;
	serializeUint32(&destMessage[1], nextAddr.sin_addr.s_addr);
	serializeUint16(&destMessage[5], nextAddr.sin_port);
	destMessage[7] = minS;
	destMessage[8] = maxS;
}

static int writeLookupResponse(unsigned char *destMessage, unsigned char *ssn, unsigned char *name, unsigned char *email, struct NetNode *netNode)
{
	int nameLen = strlen((char *)name);
	int emailLen = strlen((char *)email);

	destMessage[0] = VAL_LOOKUP_RESPONSE;
	memcpy(&destMessage[1], ssn, SSN_LENGTH);
	destMessage[1 + SSN_LENGTH] = nameLen;
	memcpy(&destMessage[2 + SSN_LENGTH], name, nameLen);
	destMessage[2 + SSN_LENGTH + nameLen] = emailLen;
	memcpy(&destMessage[3 + SSN_LENGTH + nameLen], email, emailLen);

	return 3 + SSN_LENGTH + nameLen + emailLen;
}

static void writeValInsertMessage(unsigned char *message, const char *ssn, const char *name, const char *email)
{
	unsigned char nameLen = strlen(name);
	unsigned char emailLen = strlen(email);

	message[0] = VAL_INSERT;
	memcpy(&message[1], (unsigned char *)ssn, SSN_LENGTH);
	message[1 + SSN_LENGTH] = nameLen;
	memcpy(&message[2 + SSN_LENGTH], (unsigned char *)name, nameLen);
	message[2 + SSN_LENGTH + nameLen] = emailLen;
	memcpy(&message[3 + SSN_LENGTH + nameLen], (unsigned char *)email, emailLen);
}

static void writeNetLeavingMessage(unsigned char *message, struct sockaddr_in addr)
{
	message[0] = NET_LEAVING;
	serializeUint32(&message[1], addr.sin_addr.s_addr);
	serializeUint16(&message[5], addr.sin_port);
}

static void writeArgvMessage(unsigned char *message, char *addr, char *port)
{
	int addrLen = strlen(addr);
	int portLen = strlen(port);

	memcpy(message, addr, addrLen);
	message[addrLen] = '\0';
	memcpy(&message[addrLen + 1], port, portLen);
}

static void transferUpperRange(struct NetNode *netNode, uint8_t minS, uint8_t maxS)
{
	if (!list_is_empty(netNode->entries))
	{
		ListPos pos = list_first(netNode->entries);
		while (!list_pos_equal(pos, list_end(netNode->entries)))
		{
			const char *ssn = list_inspect_ssn(pos);
			hash_t hash = hash_ssn((char *)ssn);
			if (hash >= minS && hash <= maxS)
			{
				const char *name = list_inspect_name(pos);
				const char *email = list_inspect_email(pos);
				int nameLen = strlen(name);
				int emailLen = strlen(email);
				size_t messageSize = 3 + SSN_LENGTH + nameLen + emailLen;
				unsigned char insertMessage[messageSize];
				writeValInsertMessage(insertMessage, ssn, name, email);

				if (send(netNode->fds[TCP_SOCKET_B].fd, insertMessage, messageSize, 0) == -1)
				{
					exit_on_error("Could not send to successor", netNode);
				}
				pos = list_remove(pos);
			}
			else
			{
				pos = list_next(pos);
			}
		}
	}
}

void sig_handler(int signum)
{
	if (signum == 2)
	{
		printf("\tClose requested!\n");
	}
}

static uint32_t deserializeUint32(unsigned char *message)
{
	uint32_t value = 0;

	value |= message[0] << 24;
	value |= message[1] << 16;
	value |= message[2] << 8;
	value |= message[3];

	return value;
}

static uint16_t deserializeUint16(unsigned char *message)
{
	uint16_t value = 0;

	value |= message[0] << 8;
	value |= message[1];

	return value;
}

static void serializeUint16(unsigned char *message, uint16_t value)
{
	memcpy(message, &value, 2);
}

static void serializeUint32(unsigned char *message, uint32_t value)
{
	memcpy(message, &value, 4);
}

static void removeMsgFromBuffer(unsigned char *buffer, int size)
{
	unsigned char temp[BUFF_SIZE - size];
	memcpy(temp, &buffer[size], BUFF_SIZE - size);
	memcpy(buffer, temp, BUFF_SIZE - size);
}

static void printAddress(struct sockaddr_in addr)
{
	printf("%s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

// --------- DEBUG FUNCTIONS ---------- //

// static void fprintNetJoinResponse(unsigned char *response)
// {
// 	struct sockaddr_in addr;
// 	addr.sin_family = AF_INET;
// 	addr.sin_addr.s_addr = htonl(deserializeUint32(&response[1]));
// 	addr.sin_port = htons(deserializeUint16(&response[5]));

// fprintf(stderr, "type: %d, ", response[0]);
// fprintf(stderr, "%s:%d, ", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
// fprintf(stderr, "rStart: %d, rEnd: %d\n", response[7], response[8]);
// }

// static void fprint_address(struct NetNode *netNode, int socket)
// {
// 	if (socket == UDP_SOCKET_A)
// 	{
// 		fprintf(stderr, "UDP A: ");
// 	}
// 	else if (socket == TCP_SOCKET_B)
// 	{
// 		fprintf(stderr, "TCP B: ");
// 	}
// 	else if (socket == TCP_SOCKET_C)
// 	{
// 		fprintf(stderr, "TCP C: ");
// 	}
// 	else if (socket == TCP_SOCKET_D)
// 	{
// 		fprintf(stderr, "TCP D: ");
// 	}
// 	else if (socket == UDP_SOCKET_A2)
// 	{
// 		fprintf(stderr, "UDP A2: ");
// 	}
// 	fprintf(stderr, "%s:%d\n", inet_ntoa(netNode->fdsAddr[socket].sin_addr), ntohs(netNode->fdsAddr[socket].sin_port));
// }

// static void printByteArray(unsigned char *buffer, int size)
// {
// 	for (int c = 0; c < size; c++)
// 	{
// 		fprintf(stderr, "%d ", buffer[c]);
// 	}
// 	fprintf(stderr, "\n");
// }

// static void fprint_all_sockets(struct NetNode *netNode)
// {
// 	fprintf(stderr, "fd: %d ", netNode->fds[UDP_SOCKET_A].fd);
// 	fprint_address(netNode, UDP_SOCKET_A);
// 	fprintf(stderr, "fd: %d ", netNode->fds[TCP_SOCKET_B].fd);
// 	fprint_address(netNode, TCP_SOCKET_B);
// 	fprintf(stderr, "fd: %d ", netNode->fds[TCP_SOCKET_C].fd);
// 	fprint_address(netNode, TCP_SOCKET_C);
// 	fprintf(stderr, "fd: %d ", netNode->fds[TCP_SOCKET_D].fd);
// 	fprint_address(netNode, TCP_SOCKET_D);
// 	fprintf(stderr, "fd: %d ", netNode->fds[UDP_SOCKET_A2].fd);
// 	fprint_address(netNode, UDP_SOCKET_A2);
// }