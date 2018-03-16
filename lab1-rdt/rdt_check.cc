#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_check.h"

void rdt_addchecksum(struct packet *pkt) {
    unsigned short *buf = (unsigned short *)pkt->data;
    unsigned long sum = 0;
    int i = 0;
    for (; i < RDT_PKTSIZE / sizeof(short); i++)
        sum += *buf++;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    sum = ~sum;
    sum = sum & 0xffff;
    memcpy((char *)pkt->data + RDT_PKTSIZE - sizeof(short), &sum, sizeof(short));
}

bool rdt_check(struct packet *pkt) {
	unsigned short checksum;
	memcpy(&checksum, (char *)pkt->data + RDT_PKTSIZE - sizeof(short), sizeof(short));
	memset((char *)pkt->data + RDT_PKTSIZE - sizeof(short), 0, sizeof(short));
	
	unsigned short *buf = (unsigned short *)pkt->data;
    unsigned long sum = 0;
    int i = 0;
    for (; i < RDT_PKTSIZE / sizeof(short); i++)
        sum += *buf++;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    sum = ~sum;
    sum = sum & 0xffff;
    return checksum == sum;
}