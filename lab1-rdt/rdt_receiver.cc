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
#include <iostream>

#include "rdt_struct.h"
#include "rdt_receiver.h"

#define MAX(a, b) a > b ? a : b

using namespace std;

static int ptr_left;
static int ptr_right;
const int maxpayload_size = 123;
const int buf_size = 10;
static message* buf[buf_size];
const int head_size = 1;
const int num_size = 2;
static char win_info[buf_size];

static void Receiver_makeack(short num) {
    //cout << "ack 1" << endl;
    struct packet *pkt = (struct packet*) malloc(sizeof(struct packet));
    //cout << "ack 3" << endl;
    short cnt = 0xff;
    memcpy(pkt->data, &cnt, sizeof(short));
    memcpy((char *)pkt->data + head_size, &num, sizeof(short));
    memset((char *)pkt->data + head_size + num_size, 0, maxpayload_size + 2);
    //cout << "ack 2" << endl;
    // TO DO: add check sum
    Receiver_ToLowerLayer(pkt);
    //cout << "ack 4" << endl;
}

static void Receiver_getpacket(short payload_size, struct packet *pkt, short *num, char *data) {
    //cout << "receiver getpkt1" << endl;
    memcpy(num, (char *)pkt->data + head_size, sizeof(short));
    memcpy(data, (char *)pkt->data + head_size + num_size, payload_size);
    //cout << "receiver getpkt2" << endl;
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
    //cout << "form lower layer 1" << endl;
    short num, check_sum = 0;;
    struct message *msg = (struct message*) malloc(sizeof(struct message));
    ASSERT(msg != NULL);
    //cout << "form lower layer 2" << endl;
    memcpy(&msg->size, pkt->data, head_size);
    msg->size &= 0xff;
    //cout << "msg->size " << msg->size << endl;
    msg->data = (char *)malloc(msg->size);

    Receiver_getpacket(msg->size, pkt, &num, msg->data);
    //cout << "from lower layer 2" << endl;
    //cout << num << " " << ptr_left << " " << ptr_right << "info" << endl;
    //cout << "receive num" << num << endl;
    if (num < ptr_left || num >= ptr_left + buf_size) {
        if (num < ptr_left)
            Receiver_makeack(num);

        if (msg != NULL && msg->data != NULL) free(msg->data);
        if (msg != NULL) free(msg);
        return;
    }

    else if (num == ptr_left){
        Receiver_makeack(num);
        Receiver_ToUpperLayer(msg);
        ASSERT(buf[ptr_left % buf_size] == 0);
        ptr_left++;

        while (ptr_left < ptr_right && win_info[ptr_left % buf_size] == (char) 1) {
            //cout << "ptr_left" << ptr_left << " ptr_right" << ptr_right << endl;
            ASSERT(buf[ptr_left % buf_size] != NULL);
            win_info[ptr_left % buf_size] = (char)0;
            msg = buf[ptr_left % buf_size];
            Receiver_ToUpperLayer(msg);

            buf[ptr_left % buf_size] = NULL;
            if (msg != NULL && msg->data != NULL) free(msg->data);
            if (msg != NULL) free(msg);
            
            ptr_left++;
        }
        ptr_right = MAX(ptr_left, ptr_right);
    }

    else if (num > ptr_left && num < ptr_left + buf_size) {
        //cout << "wrong?" << endl;
        Receiver_makeack(num);
        buf[num % buf_size] = msg;
        win_info[num % buf_size] = (char)1;
        ptr_right = MAX(ptr_right, num + 1);
    }
    //cout << "from lower layer 3" << endl;
}
