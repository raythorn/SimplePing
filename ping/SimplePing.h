#ifndef __SIMPLEPING_H
#define __SIMPLEPING_H

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <string.h>

#define SP_FREE(ptr) {if(ptr) free(ptr); ptr = nullptr;}
#define SP_DELETE(ptr) {if(ptr) delete ptr; ptr = nullptr;}

#define ICMP_ECHO       8
#define ICMP_ECHOREPLY  0

typedef struct __ICMPHDR {
    uint8_t icmp_type;
    uint8_t icmp_code;
    uint16_t icmp_cksum;
    uint16_t icmp_id;
    uint16_t icmp_seq;
} icmphdr_t;

typedef struct __IPHDR {
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t     ip_hl:4,        /* header length */
                ip_v:4;         /* version */
#elif BYTE_ORDER == BIG_ENDIAN
    uint8_t     ip_v:4,         /* version */
                ip_hl:4;        /* header length */
#endif
    uint8_t     ip_tos;         /* type of service */
    uint16_t    ip_len;         /* total length */
    uint16_t    ip_id;          /* identification */
    uint16_t    ip_off;         /* fragment offset field */
    uint8_t     ip_ttl;         /* time to live */
    uint8_t     ip_p;           /* protocol */
    uint16_t    ip_sum;         /* checksum */
    uint32_t    ip_src;         /* source address */
    uint32_t    ip_dst;         /* destination address */
} iphdr_t;

class SimplePing;
class ICMPPacket {
public:
    ICMPPacket() : state(0), rtt(0), bytes(0), ttl(0), tm_send(0), tm_recv(0) {
        memset(&icmphdr, 0, sizeof(icmphdr));
    }
    ~ICMPPacket() {}

    icmphdr_t   icmphdr;
    int16_t     state;
    uint16_t    rtt;
    uint8_t     bytes;
    uint8_t     ttl;
    uint64_t    tm_send;
    uint64_t    tm_recv;
};

class PingResult {
public:
    PingResult() : state(0), send(0), recv(0), loss(0), maxrtt(0), minrtt(0), avgrtt(0) {}
    ~PingResult() {}

public:
	int16_t		state;		//Ping state
	uint16_t	send;		//Total icmp packets send
	uint16_t	recv;		//Total icmp packets received
	float		loss;		//ICMP packets loss ratio
	uint16_t	maxrtt;		//Maximum RTT(Round Trip Time)
	uint16_t	minrtt;		//Minimum RTT
	uint16_t	avgrtt;		//Average RTT
};

class PingContext {
public:
    PingContext() : hostname(""), address(""), ip(0), ttl(0), count(0), socket(0), seq(0), pinger(nullptr) {}
    ~PingContext() {}

public:
    std::string	hostname;	//Domain or ip
    std::string	address;	//IP
	uint32_t	ip; 		//IP in bytes
	uint8_t		ttl;		//TTL
	uint16_t	count;		//Total icmp packets requested to send
    int         socket;
    int         seq;
	SimplePing*	pinger;		//SimplePing instance
};

class SimplePingDelegate {
public:
	~SimplePingDelegate() {}

    virtual void simplePing(int state, std::string message) = 0;
};

class SimplePing {
public:
	SimplePing();
	~SimplePing();

	enum SPNetState {
		SPNSERROR 		= -1,	//General error
		SPNSDNSUNREACH	= -2,	//DNS server unreachable
		SPNSHOSTUNREACH	= -3, 	//Host unreachable
		SPNSHOSTDOWN	= -4,	//Host is down
        SPNSHOSTNOTFOUND= -5,
		SPNSNETUNREACH	= -6,	//Network unreachable
		SPNSNETDOWN		= -7,	//Network is down
		SPNSTIMEOUT		= -8,	//Timeout
		SPNSNOMEM		= -9,	//Out of memory
		SPNSIO			= -10,	//IO error

		SPNSOK 			= 0,

		SPNSPINGINIT,		//Show ping start message
		SPNSPING,			//Show ping packet message
		SPNSPINGSTATISTIC,	//Show ping statistic message

		SPNSPINGRESULT,		//Send ping result, with json string
	};

    void ping(std::string host, int count = 3);
	
	inline void setDelegate(SimplePingDelegate *_delegate) { delegate = _delegate; }
	std::string stringfyResult(PingResult *result);
protected:
	int opensocket();
	void closesocket();
	void icmp_pack(icmphdr_t *icmphdr, int seq);
	int icmp_unpack(const char *data, int length);
    bool icmp_send();
    bool icmp_recv();
    void icmp_statistic();
	uint16_t checksum(uint16_t *data, int length);
    uint64_t timestamp();
	ICMPPacket *icmp_packet(int seq = -1);
    void icmp_packet_clear();

	static void *ping_task(void *args);

    void handPingFail(int state, std::string message = "");
private:
	PingContext *context;
    std::vector<ICMPPacket*> packets;
	std::map<int, std::string> messages;
	SimplePingDelegate *delegate;
};

#endif //__LJ_SIMPLEPING_H
