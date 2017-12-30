#include "SimplePing.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <sstream>
#include <cstring>
#include <errno.h>

using namespace std;

SimplePing::SimplePing()
{
	context = nullptr;
	packets.clear();
}

SimplePing::~SimplePing()
{
	SP_FREE(context);
	vector<icmp_packet_t*>::iterator iter = packets.begin();
	for (; iter != packets.end(); iter++) {
		SP_FREE(*iter);
	}
	packets.clear();
}

void SimplePing::ping(string host, int count, int timeout)
{
	if (nullptr == context) {
		context = (ping_context_t*)malloc(sizeof(ping_context_t));
		if (nullptr == context) {
			handPingFail(net_state_nomem, "Out of memory");
			return;
		}
		memset(context, 0, sizeof(ping_context_t));
		context->hostname = host;
		context->count = count;
		context->timeout = timeout;
		context->ttl = 64;
        context->pinger = this;
	}

	pthread_t tid;
	if (pthread_create(&tid, nullptr, SimplePing::ping_task, context))
	{
		handPingFail(net_state_task, "Start ping task fail");
		return;
	}
}

int SimplePing::opensocket()
{
	int32_t sock = 0;
	int32_t size = 128 * 1024;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	if (sock < 0) {
        handPingFail(net_state_io, "Open socket fail");
		return 0;
	}

	int ttl = 64;
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof(size));
	setsockopt(sock, IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(ttl));
	return sock;
}

void SimplePing::closesocket(int sock)
{
	close(sock);
}

void SimplePing::icmp_pack(icmphdr_t* icmphdr, int seq, int length)
{
    icmphdr->icmp_type = ICMP_ECHO;
    icmphdr->icmp_code = 0;
    icmphdr->icmp_cksum = 0;
    icmphdr->icmp_seq = seq;
    icmphdr->icmp_id = getpid();
    icmphdr->icmp_cksum = checksum((uint16_t*)icmphdr, length);
}

int SimplePing::icmp_unpack(const char *data, int length)
{
    icmphdr_t *icmphdr = (icmphdr_t*)data;
    if (nullptr == icmphdr) {
        return -1;
    }
    
    icmp_packet_t *packet = icmp_packet(icmphdr->icmp_seq);
    if (nullptr == packet) {
        return -1;
    }
    
    if (icmphdr->icmp_type != ICMP_ECHOREPLY || icmphdr->icmp_id != packet->icmphdr.icmp_id) {
        return -1;
    }
    
    packet->tm_recv = timestamp();
    uint64_t rtt = packet->tm_recv - packet->tm_send;
    stringstream ss;
    ss << "Reply from" << context->address << ": bytes" << packet->bytes << " time=" << rtt << " TTL=" << packet->ttl;
    if (delegate) {
        delegate->simplePingMessage(msg_state_ping, ss.str());
    }
    
    return 0;
}

uint16_t SimplePing::checksum(uint16_t *data, int length)
{
	uint32_t cksum = 0;
	uint16_t *src = data;
	while (length > 1) {
		cksum += *src++;
		length -= sizeof(uint16_t);
	}

	if (length == 1) {
		cksum += *(uint8_t*)src;
	}

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	return (uint16_t)(~cksum);
}

uint64_t SimplePing::timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    
    uint64_t ts = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return ts;
}

icmp_packet_t *SimplePing::icmp_packet(int seq)
{
    icmp_packet_t *packet = nullptr;
    if (seq < 0) {
        for (vector<icmp_packet_t*>::iterator iter = packets.begin(); iter != packets.end(); iter++) {
            if ((*iter)->icmphdr.icmp_seq == seq) {
                packet = *iter;
                break;
            }
        }
    } else {
        packet = (icmp_packet_t*)malloc(sizeof(icmp_packet_t));
        if (nullptr == packet) {
            handPingFail(net_state_nomem, "Out of memory");
            return nullptr;
        }
        memset(packet, 0, sizeof(icmp_packet_t));
        packet->bytes = sizeof(icmphdr_t);
        packet->ttl = context->ttl;
        packet->tm_send = timestamp();
    }

    return packet;
}

bool SimplePing::icmp_send(int sock, struct sockaddr* addr, int seq)
{
    icmp_packet_t *pkt = icmp_packet();
    if (pkt) {
        icmp_pack(&pkt->icmphdr, seq, sizeof(icmphdr_t));
        ssize_t bytes = sendto(sock, (const void *)&pkt->icmphdr, sizeof(icmphdr_t), 0, addr, sizeof(struct sockaddr));
        if (bytes < 0) {
            switch (errno) {
                case EHOSTUNREACH:
                    pkt->state = net_state_host_unreach;
                    break;
                case ENETDOWN:
                case EHOSTDOWN:
                    pkt->state = net_state_intf_down;
                    break;
                case ENETUNREACH:
                    pkt->state = net_state_route_unreach;
                default:
                    pkt->state = net_state_unknown;
                    break;
            }
            return false;
        }
    }
    
    return true;
}

bool SimplePing::icmp_recv(int sock, int seq)
{
    icmp_packet_t *pkt = icmp_packet(seq);
    if (nullptr == pkt) return false;
    
    char buffer[1024] = { 0 };
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500 * 1000; // 400 ms
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(sock, &rset);
    int ret = select(sock + 1, &rset, nullptr, nullptr, &tv);
    switch (ret) {
        case -1:
            break;
        case 0:
            pkt->state = net_state_timeout;
            break;
        default:
        {
            if (FD_ISSET(sock, &rset)) {
                ssize_t bytes = recv(sock, buffer, sizeof(buffer), 0);
                if (bytes < 0) {
                    pkt->state = net_state_io;
                    break;
                }
                if (0 == icmp_unpack(buffer, (int)bytes)) {
                    return true;
                }
            }
            break;
        }
    }
    
    return false;
}

void *SimplePing::ping_task(void *args)
{
    pthread_detach(pthread_self());
    do {
        ping_context_t *ctx = (ping_context_t*)args;
        if (nullptr == ctx) {
            break;
        }
        
        SimplePing *pinger = (SimplePing*)ctx->pinger;
        if (nullptr == pinger) {
            break;
        }
        
        struct sockaddr_in addr;
        ctx->ip = inet_addr(ctx->hostname.c_str());
        if (ctx->ip == INADDR_NONE) {
            struct hostent *host = gethostbyname(ctx->hostname.c_str());
            if (nullptr == host) {
                break;
            }
            memcpy((char*)&addr.sin_addr, host->h_addr, host->h_length);
        } else {
            memcpy((char*)&addr.sin_addr, &ctx->ip, sizeof(ctx->ip));
        }
        
        int sock = pinger->opensocket();
        if (sock < 0) {
            break;
        }
        
        uint16_t send = 0, recv = 0;
        uint16_t icmp_seq = 1;
        struct timeval begin, end;
        gettimeofday(&begin, nullptr);
        while (true) {
            gettimeofday(&end, nullptr);
            uint16_t elapse = (uint16_t)((end.tv_sec * 1000 + end.tv_usec / 1000) - (begin.tv_sec * 1000 + begin.tv_usec / 1000));
            if (elapse > ctx->timeout) {
                break;
            }

            if (send < ctx->count) {
                pinger->icmp_send(sock, (struct sockaddr *)&addr, icmp_seq++);
                send++;
            }
            
            if (pinger->icmp_recv(sock, icmp_seq)) recv++;
            if (recv == send) {
                break;
            }
        }
    } while (0);
    
    pthread_exit((void*)0);
    return nullptr;
}

void SimplePing::handPingFail(int state, string message)
{
	if (delegate)
	{
		delegate->simplePingFail(state, message);
		ping_result_t result;
		memset(&result, 0, sizeof(result));
		result.state = state;
		delegate->simplePingResult(&result);
	}
}
