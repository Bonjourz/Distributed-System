#ifndef _RDT_CHECK_H_
#define _RDT_CHECK_H_

void rdt_addchecksum(struct packet *pkt);
bool rdt_check(struct packet *pkt);

#endif