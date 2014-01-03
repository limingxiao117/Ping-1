#include "stdafx.h"
#include "rawping.h"
#include "ip_checksum.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>


int setup_for_ping(char* host,int ttl,SOCKET& sd, sockaddr_in& dest){
	// Create the sock
	sd = WSASocket(AF_INET,SOCK_RAW,IPPROTO_ICMP,0,0,0);
	if(sd == INVALID_SOCKET){
		fprintf(stdout,"Failed to create raw socket: %d\n",WSAGetLastError());
		return -1;
	}

	if(setsockopt(sd,IPPROTO_IP,IP_TTL,(const char*)&ttl,sizeof(ttl)) == SOCKET_ERROR){
		fprintf(stdout,"TTL setsockopt failed: %d\n",WSAGetLastError());
		return -1;
	}


	memset(&dest,0,sizeof(dest));
	unsigned int addr = inet_addr(host);
	if(addr != INADDR_NONE){
		dest.sin_addr.S_un.S_addr = addr;
		dest.sin_family = AF_INET;
	}else{
		hostent* hp = gethostbyname(host);
		if(hp != 0){
			memcpy(&(dest.sin_addr),hp->h_addr_list[0],hp->h_length);
			dest.sin_family = AF_INET;
		}else{
			fprintf(stdout,"failed to resolve %s\n",host);
			return -1;
		}
	}
	return 0;
}



void init_ping_packet(ICMPHeader* icmp_hdr, int packet_size, int seq_no){
	// Set up the packet's fieds
	icmp_hdr->type = ICMP_ECHO_REQUEST;
	icmp_hdr->code = 0;
	icmp_hdr->checksum = 0;
	icmp_hdr->id = (USHORT)GetCurrentProcessId();
	icmp_hdr->seq=seq_no;
	icmp_hdr->timestamp = GetTickCount();

	// You're dead meat now, packet!!!
	const unsigned long int deadmeat =0xDEADBEEF;
	char* datapart = (char*)icmp_hdr + sizeof(ICMPHeader);
	int bytes_left = packet_size - sizeof(ICMPHeader);
	while(bytes_left > 0){
		memcpy(datapart,&deadmeat,min(int(sizeof(deadmeat)),bytes_left));
		bytes_left -= sizeof(deadmeat);
		datapart += sizeof(deadmeat);
	}
	icmp_hdr->checksum = ip_checksum((USHORT*)icmp_hdr,packet_size);
}


int send_ping(SOCKET sd, const sockaddr_in& dest, 
			  ICMPHeader* send_buf, int packet_size){
	printf("Sending %d bytes to %s ...\n",packet_size,inet_ntoa(dest.sin_addr));
	int bwrote = sendto(sd,(char*)send_buf,packet_size,0,(sockaddr*)&dest,sizeof(dest));
	if(bwrote == SOCKET_ERROR){
		fprintf(stdout,"send failed : %d",WSAGetLastError());
		return -1;
	}else if(bwrote < packet_size){
		printf("send %d bytes...\n",bwrote);
	}
	return 0;
}


int recv_ping(SOCKET sd,sockaddr_in& source,IPHeader* recv_buf,
			  int packet_size){
	// Wait for the ping reply
	int fromlen = sizeof(source);
	int bread = recvfrom(sd,(char*)recv_buf,packet_size + sizeof(IPHeader),
		0, (sockaddr*)&source, &fromlen);
	if(bread == SOCKET_ERROR){
		fprintf(stdout,"read fail;");
		if(WSAGetLastError() == WSAEMSGSIZE){
			fprintf(stdout,"buffer too small\n");
		}else{
			fprintf(stdout,"error #%d\n",WSAGetLastError());
		}
	}
	return 0;
}


int decode_reply(IPHeader* reply, int bytes, sockaddr_in* from){
	// Skip ahead to the ICMP header within the IP packet
	unsigned short header_len = reply->h_len * 4;
	ICMPHeader* icmphdr = (ICMPHeader*)((char*)reply + header_len);

	// Make sure the reply is sane
	if(bytes < header_len + ICMP_MIN){
		fprintf(stdout,"too few bytes from %s \n",inet_ntoa(from->sin_addr));
		return -1;
	}else if(icmphdr->type != ICMP_ECHO_REPLY){
		if(icmphdr->type != ICMP_TTL_EXPIRE){
			if(icmphdr->type == ICMP_DEST_UNREACH){
				fprintf(stdout,"Destination unreachable\n");
			}else{
				fprintf(stdout,"Unknown ICMP packet type %d",int(icmphdr->type));
			}
			return -1;
		}
	}else if(icmphdr->id != (USHORT)GetCurrentProcessId()){
		// Must be a replay for another pinger running locally, so just ignore it
		return -2;
	}
	// À§»ó£¡£¡£¡£¡
	int nHops = int(256 - reply->ttl);
	if(nHops == 192){
		nHops = 1;
	}else if(nHops == 128){
		nHops = 0;
	}

	// Okay,now the packet must be legal -- dump it
	printf("\n%d bytes from %s , icmp_seq %d, ",bytes,inet_ntoa(from->sin_addr),
		int(icmphdr->seq));
	if(icmphdr->type == ICMP_TTL_EXPIRE)
		printf("TTL expired. \n");
	else{
		printf("%d hops, time: %ul ms.",nHops,(GetTickCount() - icmphdr->timestamp));
	}
	return 0;

}