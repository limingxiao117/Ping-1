// Ping.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "rawping.h"
#include <winsock2.h>

#define DEFAULT_PACKET_SIZE 32
#define DEFAULT_TTL 30
#define MAX_PING_DATA_SIZE 1024
#define MAX_PING_PACKET_SIZE (MAX_PING_DATA_SIZE + sizeof(IPHeader))

int allocate_buffers(ICMPHeader* &send_buf, IPHeader* &recv_buf,
					 int packet_size);


int _tmain(int argc, _TCHAR* argv[])
{
	int seq_no = 0;
	ICMPHeader* send_buf = 0;
	IPHeader* recv_buf = 0;
	if(argc < 2){
		fprintf(stdout,"usage : %s <host> [data_size] [ttl]\n",(char*)argv[0]);
		fprintf(stdout,"\tdata_size can be up to %d bytes. Default is %d.",MAX_PING_DATA_SIZE,DEFAULT_PACKET_SIZE);
		fprintf(stdout,"\tttl should be 255 or lower. Default is %d.",DEFAULT_TTL);
		return 1;
	}
	int packet_size = DEFAULT_PACKET_SIZE;
	int ttl = DEFAULT_TTL;
	if(argc > 2){
		int temp = atoi((char*)argv[2]);
		if(temp != 0){
			packet_size = temp;
		}
		if(argc > 3){
			temp = atoi((char*)argv[3]);
			if(temp > 0 && temp <= 255){
				ttl = temp;
			}
		}
	}
	packet_size = max(sizeof(ICMPHeader),
		min(MAX_PING_DATA_SIZE,(unsigned int)packet_size));

	// Start wsaData
	WSAData wsaData;
	if(WSAStartup(MAKEWORD(2,1),&wsaData) != 0){
		fprintf(stdout,"Failed to find Winsock2.1 or better.\n");
		return 1;
	}
	SOCKET sd;
	sockaddr_in dest,source;
	if(setup_for_ping((argv[1]),ttl,sd,dest) < 0){
		goto cleanup;
	}
	if(allocate_buffers(send_buf,recv_buf,packet_size) < 0){
		goto cleanup;
	}

	init_ping_packet(send_buf,packet_size,seq_no);
	// Send the ping and receive the reply
	if(send_ping(sd,dest,send_buf,packet_size) >= 0){
		while(1){
			if(recv_ping(sd,source,recv_buf,MAX_PING_PACKET_SIZE) < 0){
				unsigned short header_len = recv_buf->h_len * 4;
				ICMPHeader* icmphdr = (ICMPHeader*)((char*)recv_buf + header_len);
				if(icmphdr->seq != seq_no){
					fprintf(stdout,"bad sequence number.\n");
					continue;
				}else{
					break;
				}
			}
			if(decode_reply(recv_buf,packet_size,&source) != -2){
				break;
			}
		}
	}


cleanup:
	free(send_buf);
	free(recv_buf);
	WSACleanup();

	system("pause");
	return 0;
}



int allocate_buffers(ICMPHeader* &send_buf, IPHeader* &recv_buf, int packet_size){
	send_buf = (ICMPHeader*)malloc(packet_size);
	if(send_buf == 0){
		fprintf(stdout,"Failed to allocate output buffer.\n");
		return -1;
	}
	recv_buf = (IPHeader*)malloc(MAX_PING_PACKET_SIZE);
	if(recv_buf == 0){
		fprintf(stdout,"Failed to allocate output buffer.\n");
		return -1;
	}
	return 0;
}