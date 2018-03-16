/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<- 1 byte ->|<-2 byte->|<-123 bytes->|<-2 byte->|
 *       |payload size|packet num|  payload    | checksum |
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 *
 *       The struct of ack packet is as following:
 *       |<- 1 byte ->|<-2 byte->|<-123 bytes->|<-2 byte->|
 *       |  always -1 | ack num  |  not used   | checksum |
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_check.h"

#define WINDOW_SIZE 10
#define TIMEOUT 0.2
#define HEAD_SIZE 1
#define NUM_SIZE 2
#define CHECKSUM_SIZE 2
#define MAXPALOAD_SIZE RDT_PKTSIZE - HEAD_SIZE - NUM_SIZE - CHECKSUM_SIZE

#define MAX(a, b) a > b ? a : b

static int ptr_left;
static int ptr_right;

const int buf_size = 10;
static message* buf[buf_size];

static char win_info[buf_size];

static void Receiver_makeack(short num) {
    struct packet *pkt = (struct packet*) malloc(sizeof(struct packet));
    memset(pkt->data, 0, RDT_PKTSIZE);
    short cnt = 0xff;
    memcpy(pkt->data, &cnt, sizeof(short));
    memcpy((char *)pkt->data + HEAD_SIZE, &num, sizeof(short));
    memset((char *)pkt->data + HEAD_SIZE + NUM_SIZE, 0, MAXPALOAD_SIZE + 2);
    rdt_addchecksum(pkt);
    Receiver_ToLowerLayer(pkt);
}

static void Receiver_getpacket(short payload_size, struct packet *pkt, short *num, char *data) {
    memcpy(num, (char *)pkt->data + HEAD_SIZE, sizeof(short));
    memcpy(data, (char *)pkt->data + HEAD_SIZE + NUM_SIZE, payload_size);
}


/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    ptr_left = ptr_right = 0;
    memset(buf, 0, buf_size * sizeof(message *));
    memset(win_info, 0, buf_size);
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    /* Check correctness */
    if (!rdt_check(pkt))
        return;

    struct message *msg = (struct message*) malloc(sizeof(struct message));
    memcpy(&msg->size, pkt->data, HEAD_SIZE);
    msg->size &= 0xff;
    msg->data = (char *)malloc(msg->size);
    short num;
    Receiver_getpacket(msg->size, pkt, &num, msg->data);

    /* If the number of packet is not in the window */
    if (num < ptr_left || num >= ptr_left + buf_size) {
        if (num < ptr_left)
            Receiver_makeack(num);

        if (msg != NULL && msg->data != NULL) free(msg->data);
        if (msg != NULL) free(msg);
        return;
    }

    /* If the number of packet is at the margin of the window */
    else if (num == ptr_left){
        Receiver_makeack(num);
        Receiver_ToUpperLayer(msg);
        ASSERT(buf[ptr_left % buf_size] == 0);
        ptr_left++;

        /* Move the window */
        while (ptr_left < ptr_right && win_info[ptr_left % buf_size] == (char) 1) {
            win_info[ptr_left % buf_size] = (char)0;
            msg = buf[ptr_left % buf_size];
            Receiver_ToUpperLayer(msg);

            /* Clear the buffer */
            buf[ptr_left % buf_size] = NULL;
            if (msg != NULL && msg->data != NULL) free(msg->data);
            if (msg != NULL) free(msg);
            
            ptr_left++;
        }
        ptr_right = MAX(ptr_left, ptr_right);
    }

    /* If the number of packet is in the window */
    else if (num > ptr_left && num < ptr_left + buf_size) {
        Receiver_makeack(num);
        buf[num % buf_size] = msg;
        win_info[num % buf_size] = (char)1;
        ptr_right = MAX(ptr_right, num + 1);
    }
}
