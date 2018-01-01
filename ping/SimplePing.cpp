#include "SimplePing.h"

#include <unistd.h>
#include <arpa/inet.h>
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
    delegate = nullptr;
	packets.clear();
    messages[SPNSERROR] = "General error";
    messages[SPNSDNSUNREACH] = "DNS server is unreachable";
    messages[SPNSHOSTUNREACH] = "Host is unreachable";
    messages[SPNSHOSTDOWN] = "Host is down";
    messages[SPNSHOSTNOTFOUND] = "Host not found";
    messages[SPNSNETUNREACH] = "Network is unreachable";
    messages[SPNSNETDOWN] = "Network is down";
    messages[SPNSTIMEOUT] = "Timeout";
    messages[SPNSNOMEM] = "Out of memory";
    messages[SPNSIO] = "IO error";
}

SimplePing::~SimplePing()
{
	SP_DELETE(context);
    icmp_packet_clear();
}

void SimplePing::ping(string host, int count)
{
	if (nullptr == context) {
		context = new PingContext;
		if (nullptr == context) {
			return;
		}
	}

    context->hostname = host;
    context->count = count;
    context->ttl = 64;
    context->pinger = this;
    icmp_packet_clear();

	pthread_t tid;
	if (pthread_create(&tid, nullptr, SimplePing::ping_task, context))
	{
		handPingFail(SPNSERROR, "Start ping task fail");
		return;
	}
}

int SimplePing::opensocket()
{
	int sock = 0;
	int32_t size = 128 * 1024;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	if (sock < 0) {
        char szText[128] = {0};
        sprintf(szText, "Open socket fail, errno: %d", errno);
//        handPingFail(SPNSIO, "Open socket fail");
        handPingFail(SPNSIO, szText);
		return 0;
	}

	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof(size));
//    setsockopt(sock, IPPROTO_IP, IP_TTL, (const char *)&context->ttl, sizeof(context->ttl));
	return sock;
}

void SimplePing::closesocket()
{
    if (context->socket > 0) {
        close(context->socket);
    }
}

void SimplePing::icmp_pack(icmphdr_t *icmphdr, int seq)
{
    int length = sizeof(icmphdr_t);

    icmphdr->icmp_type = ICMP_ECHO;
    icmphdr->icmp_code = 0;
    icmphdr->icmp_cksum = 0;
    icmphdr->icmp_seq = seq;
    icmphdr->icmp_id = getpid();
    icmphdr->icmp_cksum = checksum((uint16_t*)icmphdr, length);
}

int SimplePing::icmp_unpack(const char *data, int length)
{
    int iphdrlen = 0;
    iphdr_t *iphdr = (iphdr_t*)data;
    iphdrlen = iphdr->ip_hl << 2;
    icmphdr_t *icmphdr = (icmphdr_t*)(data + iphdrlen);
    if (nullptr == icmphdr) {
        return -1;
    }
    
    ICMPPacket *packet = icmp_packet(icmphdr->icmp_seq);
    if (nullptr == packet) {
        return -1;
    }
    
    packet->ttl = iphdr->ip_ttl;
    
    if (icmphdr->icmp_type != ICMP_ECHOREPLY || icmphdr->icmp_id != packet->icmphdr.icmp_id) {
        return -1;
    }
    
    packet->tm_recv = timestamp();
    packet->rtt = packet->tm_recv - packet->tm_send;
    stringstream ss;
    ss << length << " bytes from " << context->address << ": icmp_seq=" << icmphdr->icmp_seq << " ttl=" << (int)packet->ttl << " time=" << packet->rtt << " ms";
    if (delegate) {
        delegate->simplePing(SPNSPING, ss.str());
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

ICMPPacket *SimplePing::icmp_packet(int seq)
{
    ICMPPacket *packet = nullptr;
    if (seq >= 0) {
        for (vector<ICMPPacket*>::iterator iter = packets.begin(); iter != packets.end(); iter++) {
            if ((*iter)->icmphdr.icmp_seq == seq) {
                packet = *iter;
                break;
            }
        }
    } else {
        packet = new ICMPPacket;
        if (nullptr == packet) {
            handPingFail(SPNSNOMEM);
            return nullptr;
        }

        packet->bytes = sizeof(icmphdr_t);
        packet->ttl = context->ttl;
        packet->tm_send = timestamp();
        packet->state = SPNSOK;
        packets.push_back(packet);
    }

    return packet;
}

void SimplePing::icmp_packet_clear()
{
    vector<ICMPPacket*>::iterator iter = packets.begin();
    for (; iter != packets.end(); iter++) {
        SP_DELETE(*iter);
    }
    packets.clear();
}

bool SimplePing::icmp_send()
{
    ICMPPacket *pkt = icmp_packet();
    if (pkt) {
        icmp_pack(&pkt->icmphdr, context->seq);
        struct sockaddr_in saddr;
        memset(&saddr, 0, sizeof(saddr));
        memcpy((char*)&saddr.sin_addr, &context->ip, sizeof(context->ip));
        ssize_t bytes = sendto(context->socket, (const void *)&pkt->icmphdr, sizeof(icmphdr_t), 0, (
        struct sockaddr*)&saddr, sizeof(struct sockaddr));
        if (bytes < 0) {
            char szError[16] = {0};
            sprintf(szError, "error: %d", errno);
            switch (errno) {
                case EHOSTUNREACH:
                case EHOSTDOWN:
                    pkt->state = SPNSHOSTUNREACH;
                    break;
                case ENETUNREACH:
                case ENETDOWN:
                    pkt->state = SPNSNETUNREACH;
                default:
                    pkt->state = SPNSERROR;
                    break;
            }
            
            return false;
        }
    }
    
    return true;
}

bool SimplePing::icmp_recv()
{
    ICMPPacket *pkt = icmp_packet(context->seq);
    if (nullptr == pkt) return false;
    
    char buffer[1024] = { 0 };
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000; // 100 ms
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(context->socket, &rset);
    int ret = select(context->socket + 1, &rset, nullptr, nullptr, &tv);
    switch (ret) {
        case -1:
            pkt->state = SPNSIO;
            break;
        case 0:
            pkt->state = SPNSTIMEOUT;
            break;
        default:
        {
            if (FD_ISSET(context->socket, &rset)) {
                ssize_t bytes = recv(context->socket, buffer, sizeof(buffer), 0);
                if (bytes < 0) {
                    pkt->state = SPNSIO;
                    break;
                }
                if (0 == icmp_unpack(buffer, (int)bytes)) {
                    return true;
                }
                
            }
            break;
        }
    }
    
    if (delegate) {
        delegate->simplePing(SPNSPING, messages[pkt->state]);
    }
    
    return false;
}

void SimplePing::icmp_statistic()
{
    PingResult result;
    uint16_t rtt = 0;
    result.send = packets.size();
    result.minrtt = 30000;
    vector<ICMPPacket*>::iterator iter = packets.begin();
    while (iter != packets.end()) {
        if ((*iter)->state == SPNSOK) {
            result.recv++;
        }
        result.maxrtt = max(result.maxrtt, (*iter)->rtt);
        result.minrtt = min(result.minrtt, (*iter)->rtt);
        rtt += (*iter)->rtt;
        iter++;
    }
    if (result.send == 0) {
        result.loss = 100;
    } else {
        result.loss = (result.send - result.recv) * 100.0f / result.send;
        result.avgrtt = rtt / result.send;
    }
    stringstream ss;
    ss << "\n--- " << context->hostname << " ping statistic ---\n";
    ss << result.send << " packets transmitted, " << result.recv << " received, " << result.loss << "% packet loss, time " << rtt << "ms\n";
    ss << "rtt min/avg/max = " << result.minrtt << "/" << result.avgrtt << "/" << result.maxrtt << " ms";
    if (delegate) {
        delegate->simplePing(SPNSPINGSTATISTIC, ss.str());
        delegate->simplePing(SPNSPINGRESULT, stringfyResult(&result));
    }
}

void *SimplePing::ping_task(void *args)
{
    pthread_detach(pthread_self());
    do {
        PingContext *ctx = (PingContext*)args;
        if (nullptr == ctx) {
            break;
        }
        
        SimplePing *pinger = (SimplePing*)ctx->pinger;
        if (nullptr == pinger) {
            break;
        }

        ctx->ip = inet_addr(ctx->hostname.c_str());
        if (ctx->ip == INADDR_NONE) {
            struct hostent *host = gethostbyname(ctx->hostname.c_str());
            if (nullptr == host) {
                pinger->handPingFail(SPNSHOSTNOTFOUND);
                break;
            }
            memcpy((char*)&ctx->ip, host->h_addr, host->h_length);
        }

        struct sockaddr_in saddr;
        memcpy((char*)&saddr.sin_addr, &ctx->ip, sizeof(ctx->ip));
        
        ctx->address = inet_ntoa(saddr.sin_addr);

        ctx->socket = pinger->opensocket();
        if (ctx->socket < 0) {
            pinger->handPingFail(SPNSIO);
            break;
        }
        
        stringstream ss;
        ss << "PING " << ctx->hostname << " (" << ctx->address << ") " << 28 << " bytes of data.";
        if (pinger->delegate) {
            pinger->delegate->simplePing(SPNSPINGINIT, ss.str());
        }

        ctx->seq = 0;
        while (true) {
            if (ctx->seq == ctx->count) {
                break;
            }
            ctx->seq++;
            if (!pinger->icmp_send()) {
                continue;
            }
            pinger->icmp_recv();
        }
        pinger->icmp_statistic();
        pinger->closesocket();
    } while (0);
    
    pthread_exit((void*)0);
    return nullptr;
}

void SimplePing::handPingFail(int state, string message)
{
	if (delegate && state < 0)
	{
        delegate->simplePing(state, message.empty() ? messages[state] : message);
		PingResult result;
		memset(&result, 0, sizeof(result));
		result.state = state;
        delegate->simplePing(SPNSPINGRESULT, stringfyResult(&result));
	}
}

string SimplePing::stringfyResult(PingResult *result)
{
    stringstream ss;
    ss << "{";
    ss << "state:" << result->state << ",";
    ss << "send:" << result->send << ",";
    ss << "recv:" << result->recv << ",";
    ss << "loss:" << result->loss << ",";
    ss << "maxrtt:" << result->maxrtt << ",";
    ss << "minrtt:" << result->minrtt << ",";
    ss << "avgrtt:" << result->avgrtt;
    ss << "}";

    return ss.str();
}
