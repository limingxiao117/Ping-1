#include "stdafx.h"
#include "ip_checksum.h"


unsigned short ip_checksum(unsigned short* buffer,int size){
	unsigned long cksum = 0;
	// Sum all the words together,adding the finally byte if size is old
	while(size > 1){
		cksum += *buffer++;
		size -= sizeof(unsigned short);
	}
	if(size){
		cksum += *(unsigned char*)buffer;
	}

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (unsigned short)(~cksum);

}



