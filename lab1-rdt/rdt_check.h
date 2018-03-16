#ifndef _RDT_CHECK_H_
#define _RDT_CHECK_H_

/* Add the checksum part of the packe */
void rdt_addchecksum(struct packet *pkt);

/* Check whether data corruption happens */
bool rdt_check(struct packet *pkt);

#endif