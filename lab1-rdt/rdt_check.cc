#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_check.h"

static unsigned short getchecksum(struct packet *pkt) {
    memset((char *)pkt->data + RDT_PKTSIZE - sizeof(short), 0, sizeof(short));
    
    unsigned short *buf = (unsigned short *)pkt->data;
    unsigned long checksum = 0;
    int i = 0;
    for (; i < RDT_PKTSIZE / sizeof(short); i++)
        checksum += *buf++;

    checksum = (checksum >> 16) + (checksum & 0xffff);
    checksum += (checksum >> 16);
    checksum = ~checksum;
    checksum = checksum & 0xffff;
    return checksum;
}

void rdt_addchecksum(struct packet *pkt) {
    unsigned short checksum = getchecksum(pkt);
    memcpy((char *)pkt->data + RDT_PKTSIZE - sizeof(short), &checksum, sizeof(short));
}

bool rdt_check(struct packet *pkt) {
	unsigned short checksum;
	memcpy(&checksum, (char *)pkt->data + RDT_PKTSIZE - sizeof(short), sizeof(short));

    return checksum == getchecksum(pkt);
}