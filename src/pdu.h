#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define SSN_LENGTH 12

#define NET_ALIVE 0
#define NET_GET_NODE 1
#define NET_GET_NODE_RESPONSE 2
#define NET_JOIN 3
#define NET_JOIN_RESPONSE 4
#define NET_CLOSE_CONNECTION 5

#define NET_NEW_RANGE 6
#define NET_LEAVING 7
#define NET_NEW_RANGE_RESPONSE 8

#define VAL_INSERT 100
#define VAL_REMOVE 101
#define VAL_LOOKUP 102
#define VAL_LOOKUP_RESPONSE 103

#define STUN_LOOKUP 200
#define STUN_RESPONSE 201

#ifndef PDU_DEF
#define PDU_DEF

struct NET_ALIVE_PDU {
    uint8_t type;
};

struct NET_GET_NODE_PDU {
    uint8_t type;
};

struct NET_GET_NODE_RESPONSE_PDU {
    uint8_t type;
    uint32_t address;
    uint16_t port;
};

struct NET_JOIN_PDU {
    uint8_t type;
    uint32_t src_address;
    uint16_t src_port;
    uint8_t max_span;
    uint32_t max_address;
    uint16_t max_port;
};

struct NET_JOIN_RESPONSE_PDU {
    uint8_t type;
    uint32_t next_address;
    uint16_t next_port;
    uint8_t range_start;
    uint8_t range_end;
};

struct NET_CLOSE_CONNECTION_PDU {
    uint8_t type;
};

struct NET_NEW_RANGE_PDU {
    uint8_t type;
    uint8_t range_start;
    uint8_t range_end;
};

struct NET_NEW_RANGE_RESPONSE_PDU {
    uint8_t type;
};

struct NET_LEAVING_PDU {
    uint8_t type;
    uint32_t new_address; 
    uint16_t new_port;
};

struct VAL_INSERT_PDU {
    uint8_t type;
    uint8_t ssn[SSN_LENGTH];
    uint8_t name_length;
    uint8_t* name;
    uint8_t email_length;
    uint8_t* email;
};

struct VAL_REMOVE_PDU {
    uint8_t type;
    uint8_t ssn[SSN_LENGTH];
};

struct VAL_LOOKUP_PDU {
    uint8_t type;
    uint8_t ssn[SSN_LENGTH];
    uint32_t sender_address;
    uint16_t sender_port;
};

struct VAL_LOOKUP_RESPONSE_PDU {
    uint8_t type;
    uint8_t ssn[SSN_LENGTH];
    uint8_t name_length;
    uint8_t* name;
    uint8_t email_length;
    uint8_t* email;
};

struct STUN_LOOKUP_PDU {
    uint8_t type;
};

struct STUN_RESPONSE_PDU {
    uint8_t type;
    uint32_t address;
};

#endif
