/* 
 * Copyright (c) 2014, 2015, 2016 University of Michigan, Ann Arbor.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of Michigan, Ann Arbor. The name of the University 
 * may not be used to endorse or promote products derived from this 
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Sugih Jamin (jamin@eecs.umich.edu)
 *
*/
#ifndef __NETIMG_H__
#define __NETIMG_H__

#ifdef _WIN32
#define usleep(usec) Sleep(usec/1000)
#define ioctl(sockdesc, request, onoff) ioctlsocket(sockdesc, request, onoff)
#define perror(errmsg) { fprintf(stderr, "%s: %d\n", (errmsg), WSAGetLastError()); }
#endif
#define net_assert(err, errmsg) { if ((err)) { perror(errmsg); assert(!(err)); } }

#define NETIMG_SEED 48916

#define NETIMG_WIDTH  640
#define NETIMG_HEIGHT 480
#define NETIMG_RGBA   0x4
#define NETIMG_RGB    0x3
#define NETIMG_GSA    0x2
#define NETIMG_GS     0x1

#define NETIMG_MAXFNAME  256   // including terminating NULL
#define NETIMG_PORTSEP   ':'

#define NETIMG_MAXTRIES    3
#define NETIMG_NUMIOV      2

#define NETIMG_MAXWIN    255   // 2^8 -1
#define NETIMG_MINWIN      4
#define NETIMG_RCVWIN     12
#define NETIMG_FECWIN     11   // Lab6 & PA3
#define NETIMG_UDPIP      28   // 20 bytes IP, 8 bytes UDP headers
#define NETIMG_MSS     10276   // 10KB segments, corresponds to
                               // SO_SNDBUF/SO_RCVBUF so including
                               // the 36-byte headers (ihdr_t+UDP+IP)
#define NETIMG_MINSS      40   // 36 bytes headers, 4 bytes data
#define NETIMG_MINPROB 0.011
#define NETIMG_MAXPROB 0.11
#define NETIMG_PDROP   0.021   // recommended between NETIMG_MINPROB and 
                               // NETIMG_MAXPROB, values larger than 
                               // NETIMG_MAXPROB simulates massive drops
                               // -1.0 to turn off
#define NETIMG_SLEEP       1   // secs, set to 20 or larger for X11
                               // forwarding on CAEN over ADSL, to
                               // prevent unnecessary retransmissions
#define NETIMG_USLEEP 500000   // 500 ms

#define NETIMG_VERS    0x11

// imsg_t::img_type from client:
#define NETIMG_SYNQRY  0x10
#define NETIMG_ACK     0x11    // PA3

// imsg_t::img_type from server:
#define NETIMG_FOUND   0x02
#define NETIMG_NFOUND  0x04
#define NETIMG_ERROR   0x08
#define NETIMG_ESIZE   0x09
#define NETIMG_EVERS   0x0a
#define NETIMG_ETYPE   0x0b
#define NETIMG_ENAME   0x0c
#define NETIMG_EBUSY   0x0d

#define NETIMG_DATA    0x20
#define NETIMG_FEC     0x60    // Lab6 & PA3
#define NETIMG_FIN     0xa0    // PA3

// special seqno's for PA3:
#define NETIMG_MAXSEQ  2147483647 // 2^31-1
#define NETIMG_SYNSEQ  4294967295 // 2^32-1
#define NETIMG_FINSEQ  4294967294 // 2^32-2

typedef struct {                  // NETIMG_SYNQRY
  unsigned char iq_vers;
  unsigned char iq_type;
  unsigned short iq_mss;          // receiver's maximum segment size
  unsigned char iq_rwnd;          // receiver's window size
  unsigned char iq_fwnd;          // receiver's FEC window size
                                  // used in Lab6 and PA3
  char iq_name[NETIMG_MAXFNAME];  // must be NULL terminated
} iqry_t;

typedef struct {               
  unsigned char im_vers;
  unsigned char im_type;       // NETIMG_FOUND or NETIMG_NFOUND
  unsigned char im_depth;      // in bytes, not in bits as
                               // returned by LTGA.GetPixelDepth()
  unsigned char im_format;
  unsigned short im_width;
  unsigned short im_height;
} imsg_t;

typedef struct {
  unsigned char ih_vers;
  unsigned char ih_type;       // NETIMG_DATA
                               // Lab6: NETIMG_FEC,
                               // PA3: NETIMG_ACK, NETIMG_FIN
  unsigned short ih_size;      // actual data size, in bytes,
                               // not including header
  unsigned int ih_seqn;
} ihdr_t;

class netimg {
  unsigned short mss;       // receiver's maximum segment size, in bytes
  unsigned char rwnd;       // receiver's window, in packets, of size <= mss
  unsigned char fwnd;       // Lab6: receiver's FEC window < rwnd, in packets
  float pdrop;              // PA3 Task 2.3: probabilistically drop an ACK

  unsigned int next_seqn;   // Lab6 and PA3: next expected sequence number

public:
  int sd;                   // socket descriptor
  imsg_t imsg;
  unsigned int window_start;
  unsigned int datasize;
  int packets_count;
  bool go_back_n_mode;
  unsigned int next_next_seqn;
  netimg() {next_seqn = 0; window_start = 0; packets_count = 0; go_back_n_mode=false;}   // default constructor
  int args(int argc, char *argv[], char **sname, unsigned short *port, char **imgname);
  int rcvbuf() { return(rwnd*mss); }
  int sendqry(char *imgname);
  char recvimsg();
  void recvimg();
  void reconstruct_image(unsigned char* fec_data);
  void send_ack(ihdr_t* ack);

};

extern void netimglut_init(int *argc, char *argv[], void (*idlefunc)());
extern void netimglut_imginit(unsigned short format);

#endif /* __NETIMG_H__ */
