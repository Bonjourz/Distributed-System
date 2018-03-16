/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<- 1 byte ->|<-2 byte->|<-123 bytes->|<-2 byte->|
 *       |payload size|packet num|   payload   | checksum |
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

#include <list>
#include <vector>
#include <queue>
#include <iostream>

#include "rdt_struct.h"
#include "rdt_sender.h"
#include "rdt_check.h"
using namespace std;

static char window_info[0xffff];
static short win_left = 0;
static short win_right = 0;
static short pkt_num = 0;
static std::list<packet *>pkt_list;

#define MIN(a, b) a < b ? a : b

const short maxpayload_size = 123;

static void Sender_sendpacket() {
  win_right = MIN(win_left + WINDOW_SIZE, pkt_num);
  std::list<packet *>::iterator it = pkt_list.begin();
  for (int i = 0; i < win_right - win_left; i++) {
    if (window_info[win_left + i] == (char)0) {
      //short tmp;
      //memcpy(&tmp, (*it)->data + 1, sizeof(short));
      //cout << "send num " << tmp << endl;
      Sender_ToLowerLayer(*it);
    }
    it++;
  }
}

static void Sender_Makepack(short payload_size, short num, const char *content, packet *pkt) {
  /* only for little-endian */
  memset(pkt->data, 0, RDT_PKTSIZE);
  memcpy(pkt->data, &payload_size, sizeof(short));
  memcpy((char *)pkt->data + HEAD_SIZE, &num, sizeof(short));
  memcpy((char *)pkt->data + HEAD_SIZE + NUM_SIZE, content, payload_size);
  rdt_addchecksum(pkt);
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
  win_left = 0;
  win_right = 0;
  memset(window_info, 0, 0xffff);
  fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
  fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
  if (msg->size == 0)
    return;

  //cout << "begin sender from" << endl;
  //cout << "sender from upper" << endl;
  /* reuse the same packet data structure */
  struct packet *pkt;

  /* the cursor always points to the first unsent byte in the message */
  int cursor = 0;

  /* packet num includes the header packet */
  //int packet_num = (msg->size-1) / maxpayload_size + 2;

  while (msg->size-cursor > maxpayload_size) {
  /* fill in the packet */
    pkt = (packet *)malloc(sizeof(struct packet));
    Sender_Makepack(maxpayload_size, pkt_num, msg->data + cursor, pkt);
    ASSERT(pkt);
    pkt_list.push_back(pkt);

    /* move the cursor */
    cursor += maxpayload_size;
    pkt_num++;
  }

  /* send out the last packet */
  if (msg->size > cursor) {
  /* fill in the packet */
    pkt = (packet *)malloc(sizeof(struct packet));
    Sender_Makepack(msg->size - cursor, pkt_num, msg->data + cursor, pkt);
    ASSERT(pkt);
    pkt_list.push_back(pkt);
    pkt_num++;
  }
  //cout << "finish sender from" << endl;
  Sender_StopTimer();
  Sender_StartTimer(TIMEOUT);
  //cout << "finish sender from 123" << endl;
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
  if (!rdt_check(pkt))
    return;

  short ack_num;
  memcpy(&ack_num, (char *)pkt->data + HEAD_SIZE, sizeof(short));
  
  if (ack_num < win_left || ack_num >= win_right || ack_num >= pkt_num)
    return;

  if (Sender_isTimerSet())
    Sender_StopTimer();
  window_info[ack_num] = (char)1;
  //cout << " ack_num " << ack_num << " " << win_left << " " << win_right << " " << pkt_num << endl;

  /* Move the sliding window */
  while (win_left < pkt_num && window_info[win_left] == (char)1) {
    struct packet *pkt = pkt_list.front();
    ASSERT(pkt != NULL);
    if (pkt != NULL) free(pkt);
    pkt_list.pop_front();
    win_left++;
    ASSERT(win_left <= win_right);
  }

  Sender_StartTimer(TIMEOUT);
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
  if (win_left >= pkt_num)
    return;
  Sender_sendpacket();
  Sender_StartTimer(TIMEOUT);
}
