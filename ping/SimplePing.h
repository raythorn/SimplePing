#ifndef __SIMPLEPING_H
#define __SIMPLEPING_H

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>

#define SP_FREE(ptr) {if(ptr) free(ptr); ptr = nullptr;}
#define SP_DELETE(ptr) {if(ptr) delete ptr; ptr = nullptr;}

#define ICMP_ECHO       8
#define ICMP_ECHOREPLY  0

typedef struct __ICMPHDR {
	uint8_t		icmp_type;
	uint8_t		icmp_code;
	uint16_t	icmp_cksum;
	uint16_t	icmp_id;
	uint16_t	icmp_seq;
} icmphdr_t;

typedef struct __ICMP_PACKET {
	icmphdr_t	icmphdr;
	int16_t		state;
	uint16_t	rtt;
	uint8_t		bytes;
	uint8_t		ttl;
	uint64_t	tm_send;
	uint64_t	tm_recv;
} icmp_packet_t;

typedef struct __PING_RESULT {
	int16_t		state;		//Ping state
	uint16_t	send;		//Total icmp packets send
	uint16_t	recv;		//Total icmp packets received
	float		loss;		//ICMP packets loss ratio
	uint16_t	maxrtt;		//Maximum RTT(Round Trip Time)
	uint16_t	minrtt;		//Minimum RTT
	uint16_t	avgrtt;		//Average RTT
} ping_result_t;

typedef struct __PING_CONTEXT {
    std::string	hostname;	//Domain or ip
    std::string	address;	//IP
	uint32_t	ip; 		//IP in bytes
	uint8_t		ttl;		//TTL
	uint16_t	timeout;	//Timeout to stop ping, unit: ms
	uint16_t	count;		//Total icmp packets requested to send
	void*		pinger;		//SimplePing instance 
} ping_context_t;

class SimplePingDelegate {
public:
	~SimplePingDelegate() {}

    virtual void simplePingMessage(int state, std::string message) = 0;
	virtual void simplePingResult(ping_result_t *result) = 0;
    virtual void simplePingFail(int code, std::string error) = 0;
};

class SimplePing {
public:
	SimplePing();
	~SimplePing();

	enum SPNetState {
		net_state_ok = 0,
		net_state_dns,
        net_state_host_notfound,
        net_state_host_unreach,
        net_state_route_unreach,
        net_state_intf_down,
        net_state_timeout,
		net_state_nomem,
		net_state_task,
        net_state_io,
        net_state_unknown
	};

	enum SPMessageState {
		msg_state_begin = 0,
		msg_state_ping,
		msg_state_statistic
	};

	enum SPIcmpState {
		icmp_state_init = 0,
		icmp_state_send,
		icmp_state_recv,
	};

    void ping(std::string host, int count = 3, int timeout = 5000);
	
	inline void setDelegate(SimplePingDelegate *_delegate) { delegate = _delegate; }

protected:
	int opensocket();
	void closesocket(int sock);
	void icmp_pack(icmphdr_t* icmphdr, int seq, int length);
	int icmp_unpack(const char *data, int length);
    bool icmp_send(int sock, struct sockaddr* addr, int seq);
    bool icmp_recv(int sock, int seq);
	uint16_t checksum(uint16_t *data, int length);
    uint64_t timestamp();
	icmp_packet_t *icmp_packet(int seq = -1);

	static void *ping_task(void *args);

    void handPingFail(int state, std::string message);
private:
	ping_context_t *context;
    std::vector<icmp_packet_t*> packets;
	SimplePingDelegate *delegate;
};

#endif //__LJ_SIMPLEPING_H
