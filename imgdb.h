/* 
 * Copyright (c) 2015, 2016 University of Michigan, Ann Arbor.
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
#ifndef __IMGDB_H__
#define __IMGDB_H__

#include "ltga.h"
#include "socks.h"
#include "netimg.h"

#ifdef _WIN32
#define IMGDB_DIRSEP "\\"
#else
#define IMGDB_DIRSEP "/"
#endif
#define IMGDB_FOLDER    "."

class imgdb {
  struct sockaddr_in self;
  char sname[NETIMG_MAXFNAME];

  struct sockaddr_in client;
  float pdrop;
  unsigned short mss;  // receiver's maximum segment size, in bytes
  // used in Lab6 and PA3:
  unsigned char rwnd;  // receiver's window, in packets, each of size <= mss
  unsigned char fwnd;  // receiver's FEC window, in packets

  LTGA curimg;

  char readimg(char *imgname, int verbose);

  char recvqry(iqry_t *iqry);
  double marshall_imsg(imsg_t *imsg);
  int sendpkt(char *pkt, int size, unsigned int ackseqn, int all);
  int sendimsg(imsg_t *imsg);

public:
  int sd;  // image socket
  struct timeval timeout;
  fd_set imgdb_fd_set;

  imgdb() { // default constructor
    pdrop = NETIMG_PDROP;
    timeout.tv_sec = NETIMG_SLEEP;
    timeout.tv_usec = NETIMG_USLEEP;


    sd = socks_servinit((char *) "imgdb", &self, sname); // Task 1
  }

  int args(int argc, char *argv[]);

  // image query-reply
  void handleqry();
  void sendimg(char *image, long imgsize);
};  

#endif /* __IMGDB_H__ */
