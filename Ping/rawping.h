#ifndef PING_H
#define PING_H

#include <winsock2.h>


// ICMP package types
// 0,8 为会送回答和请求报文  报文的格式是一样的
#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8
#define ICMP_DEST_UNREACH 3 // 重点不可达
#define ICMP_TTL_EXPIRE 11 // 时间超时

// Minumum ICMP package size in bytes
#define ICMP_MIN 8

#ifdef _MSV_VER

#pragma pack(1)
#endif

struct IPHeader{
	BYTE h_len:4;
	BYTE version:4;
	BYTE tos;
	USHORT total_len;
	USHORT ident;
	USHORT flags;
	BYTE ttl;
	BYTE proto;
	USHORT checksum;
	ULONG source_ip;
	ULONG dest_ip;
};

struct ICMPHeader{
	BYTE type;
	BYTE code;
	USHORT checksum;
	USHORT id;
	USHORT seq;
	ULONG timestamp;
};

#ifdef _MSV_VER
#pragma pack()
#endif

extern int setup_for_ping(char* host, int ttl, SOCKET& sd, 
						  sockaddr_in& dest);
extern int send_ping(SOCKET sd, const sockaddr_in& dest, 
					 ICMPHeader* send_buf, int packet_size);
extern int recv_ping(SOCKET sd, sockaddr_in& source, IPHeader* recv_buf,
					 int packet_size);
extern int decode_reply(IPHeader* reply, int bytes, sockaddr_in* from);
extern void init_ping_packet(ICMPHeader* icmp_hdr, int packet_size, 
							 int seq_no);


#endif