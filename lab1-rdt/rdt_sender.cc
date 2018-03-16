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
 *       |  always -1 |packet num|  not used   | checksum |
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
using namespace std;

static std::vector<char> window_info(0xffff, 0);
static short win_left = 0;
static short win_right = 1;
static short pkt_num = 0;
static std::list<packet *>pkt_list;

const short maxpayload_size = 123;

static void Sender_sendpacket() {
  //cout << "begin sender send" << endl;
  while (win_right - win_left < WINDOW_SIZE && win_right < pkt_num) {
    std::list<packet *>::iterator it = pkt_list.begin();
    for (int i = 1; i < win_right - win_left; i++)
      it++;
    ASSERT(it != pkt_list.end());
    Sender_ToLowerLayer(*it);
    win_right++;
  }

  //cout << "mid sender send" << endl;
  //cout << win_left << " " << pkt_num << endl;
  if (win_left < pkt_num)
    Sender_StartTimer(TIMEOUT);

  //cout << "end sender send" << endl;
}

static void Sender_Makepack(short payload_size, short num, const char *content, packet *pkt) {
  /* only for little-endian */
  memset(pkt->data, 0, RDT_PKTSIZE);
  memcpy(pkt->data, &payload_size, sizeof(short));
  memcpy((char *)pkt->data + HEAD_SIZE, &num, sizeof(short));
  memcpy((char *)pkt->data + HEAD_SIZE + NUM_SIZE, content, payload_size);
  
  // TO DO: add checksum
  short check_sum = 0;
  memcpy((char *)pkt->data + HEAD_SIZE + NUM_SIZE + maxpayload_size, &check_sum, sizeof(short));
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
  cout << "size of " << window_info.size() << endl;
  cout << " " << sizeof(short) << endl;
  win_left = 0;
  win_right = 1;
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
  packet *pkt;

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
  if (!Sender_isTimerSet())
    Sender_sendpacket();
  //cout << "finish sender from 123" << endl;
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
  if (Sender_isTimerSet())
    Sender_StopTimer();

  // TO DO: correctness check
  short ack_num;
  memcpy(&ack_num, (char *)pkt->data + HEAD_SIZE, sizeof(short));
  window_info[ack_num] = (char)1;

  /* Move the sliding window */
  while (win_left < pkt_num && window_info[win_left] == (char)1) {
    struct packet *pkt = pkt_list.front();
    ASSERT(pkt != NULL);
    if (pkt != NULL) free(pkt);
    pkt_list.pop_front();
    win_left++;
    ASSERT(win_left <= win_right);
  }

  Sender_sendpacket();
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
  //printf("time out begin\n");
  for (int i = win_left; i < win_right; i++) {
    if (window_info[i] == (char)0) {
      std::list<packet *>::iterator it = pkt_list.begin();
      //cout << "ifbegin" << endl;
      for (int j = 1; j < win_right - win_left; j++)
        it++;
      //cout << "ifmid" << win_right << pkt_num << endl;
      ASSERT(it != pkt_list.end());
      Sender_ToLowerLayer(*it);
      //cout << "ifend" << endl;
    }
  }
  //printf("time out mid\n");
  Sender_sendpacket();
  //printf("time out end\n");
}
