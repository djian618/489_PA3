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
#include <stdio.h>         // fprintf(), perror(), fflush()
#include <stdlib.h>        // atoi(), random()
#include <assert.h>        // assert()
#include <limits.h>        // LONG_MAX, INT_MAX
#include <iostream>
using namespace std;
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>      // socklen_t
#include "wingetopt.h"
#else
#include <string.h>        // memset(), memcmp(), strlen(), strcpy(), memcpy()
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname(), gethostbyaddr()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons(), inet_ntoa()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API, setsockopt(), getsockname()
#include <sys/ioctl.h>     // ioctl(), FIONBIO
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "ltga.h"
#include "socks.h"
#include "netimg.h"
#include "imgdb.h"
#include <time.h>
#include <algorithm>    // std::max
#include <errno.h>
/*
 * args: parses command line args.
 *
 * Returns 0 on success or 1 on failure.  On successful return,
 * the provided drop probability is stored in imgdb::pdrop.
 *
 * Nothing else is modified.
 */
int imgdb::
args(int argc, char *argv[])
{
  char c;
  extern char *optarg;

  if (argc < 1) {
    return (1);
  }
  
  while ((c = getopt(argc, argv, "d:")) != EOF) {
    switch (c) {
    case 'd':
      pdrop = atof(optarg);
      if (pdrop > 0.0 && (pdrop > NETIMG_MAXPROB || pdrop < NETIMG_MINPROB)) {
        fprintf(stderr, "%s: recommended drop probability between %f and %f.\n", argv[0],
                NETIMG_MINPROB, NETIMG_MAXPROB);
      }
      break;
    default:
      return(1);
      break;
    }
  }

  srandom(NETIMG_SEED+(int)(pdrop*1000));

  return (0);
}

/*
 * readimg: load TGA image from file "imgname" to curimg.
 * "imgname" must point to valid memory allocated by caller.
 * Terminate process on encountering any error.
 * Returns NETIMG_FOUND if "imgname" found, else returns NETIMG_NFOUND.
 */
char imgdb::
readimg(char *imgname, int verbose)
{
  string pathname=IMGDB_FOLDER;

  if (!imgname || !imgname[0]) {
    return(NETIMG_ENAME);
  }
  
  curimg.LoadFromFile(pathname+IMGDB_DIRSEP+imgname);

  if (!curimg.IsLoaded()) {
    return(NETIMG_NFOUND);
  }

  if (verbose) {
    cerr << "Image: " << endl;
    cerr << "       Type = " << LImageTypeString[curimg.GetImageType()] 
         << " (" << curimg.GetImageType() << ")" << endl;
    cerr << "      Width = " << curimg.GetImageWidth() << endl;
    cerr << "     Height = " << curimg.GetImageHeight() << endl;
    cerr << "Pixel depth = " << curimg.GetPixelDepth() << endl;
    cerr << "Alpha depth = " << curimg.GetAlphaDepth() << endl;
    cerr << "RL encoding = " << (((int) curimg.GetImageType()) > 8) << endl;
    /* use curimg.GetPixels()  to obtain the pixel array */
  }
  
  return(NETIMG_FOUND);
}

/*
 * marshall_imsg: Initialize *imsg with image's specifics.
 * Upon return, the *imsg fields are in host-byte order.
 * Return value is the size of the image in bytes.
 *
 * Terminate process on encountering any error.
 */
double imgdb::
marshall_imsg(imsg_t *imsg)
{
  imsg->im_depth = (unsigned char)(curimg.GetPixelDepth()/8);
  if (((int) curimg.GetImageType()) == 3 ||
      ((int) curimg.GetImageType()) == 11) {
    imsg->im_format = ((int) curimg.GetAlphaDepth()) ?
      NETIMG_GSA : NETIMG_GS;
  } else {
    imsg->im_format = ((int) curimg.GetAlphaDepth()) ?
      NETIMG_RGBA : NETIMG_RGB;
  }
  imsg->im_width = curimg.GetImageWidth();
  imsg->im_height = curimg.GetImageHeight();

  return((double) (imsg->im_width*imsg->im_height*imsg->im_depth));
}

/* 
 * recvqry: receives an iqry_t packet and stores the client's address
 * and port number in the imgdb::client member variable.  Checks that
 * the incoming iqry_t packet is of version NETIMG_VERS and of type
 * NETIMG_SYNQRY.
 *
 * If error encountered when receiving packet, or if packet is of the
 * wrong version or type, returns appropriate NETIMG error code.
 * Otherwise returns 0.
 *
 * Nothing else is modified.
*/
char imgdb::
recvqry(iqry_t *iqry)
{
  int bytes;  // stores the return value of recvfrom()

  /*
   * Lab5 Task 1: Call recvfrom() to receive the iqry_t packet from
   * client.  Store the client's address and port number in the
   * imgdb::client member variable and store the return value of
   * recvfrom() in local variable "bytes".
  */
  /* Lab5: YOUR CODE HERE */
  socklen_t client_size = sizeof(client);

  bytes = recvfrom(sd, iqry, sizeof(iqry_t), 0, (struct sockaddr *) &client, &client_size);
  
  fprintf(stderr, "%s\n", "finish recived");

  if (bytes != sizeof(iqry_t)) {
    return (NETIMG_ESIZE);
  }
  if (iqry->iq_vers != NETIMG_VERS) {
    return(NETIMG_EVERS);
  }
  if (iqry->iq_type == NETIMG_SYNQRY) {
    if (strlen((char *) iqry->iq_name) >= NETIMG_MAXFNAME) {
      return(NETIMG_ENAME);
    }
  } else if (iqry->iq_type != NETIMG_ACK) {
    return(NETIMG_ETYPE);
  }
  // drop/ignore NETIMG_ACK packets

  return(0);
}


/* 
 * Lab5 Task 1:
 * sendpkt: sends the provided "pkt" of size "size"
 * to imgdb::client using sendto().
 *
 * PA3 Task 2.1:
 * If send is successful, wait for an ACK. When an ACK is received,
 * check that its sequence number is the expected "ackseqn".  If the
 * ACK packet is not received, or if the sequence number doesn't
 * match, repeat NETIMG_MAXTRIES times.  The "all" flag tells
 * sendpkt() whether to wait for only one ACK packet (0) or to
 * "opportunistically" grab all ACKs that have returned to the server
 * (1).  If "all" is 1, "ackseqn" must match the sequence number of
 * the last ACK received.
 *
 * Returns 0 if send success (and ack-ed correctly for PA3), else -1.
 *
 * Nothing else is modified.
*/
int imgdb::
sendpkt(char *pkt, int size, unsigned int ackseqn, int all)
{


  /* Lab5 and PA3 Task 2.1: YOUR CODE HERE */
    socklen_t client_size = sizeof(client);
    fd_set imgdb_sendpkt_set;
    FD_ZERO(&imgdb_sendpkt_set);
    FD_SET(sd, &imgdb_sendpkt_set);
    int count =0;
    ihdr_t ihdr_ack;
    unsigned int recived_ih_seq;
    while(count < NETIMG_MAXTRIES){
    //send packet  
    ssize_t judge = sendto(sd, pkt, size, 0, (struct sockaddr *) &client, client_size);
    assert(judge>0&&"failed sent to client");

    int err = select(sd+1, &imgdb_sendpkt_set, 0, 0, &timeout);
    bool recived_right_ack_seq = false;

    if(!err){// timeout
      
      if((recived_right_ack_seq)&all) {return 0;}
      count++;
      fprintf(stderr, "time out recive ack,tried %d times\n", count);
      continue;
    }else  if(FD_ISSET(sd, &imgdb_sendpkt_set)){
            if(all==0){
              int bytes = recvfrom(sd, &ihdr_ack, sizeof(ihdr_t), 0, (struct sockaddr *) &client, &client_size);
              net_assert((bytes<0),"recv error when all ==0");
              recived_ih_seq = ntohl(ihdr_ack.ih_seqn);
              if(recived_ih_seq == ackseqn) return 0;
              else return (-1);

            }else{
                while(1){
                  fprintf(stderr, "recive ack num is 0x%x\n", recived_ih_seq);
                  int bytes = recvfrom(sd, &ihdr_ack, sizeof(ihdr_t), MSG_DONTWAIT, (struct sockaddr *) &client, &client_size);
                  if(bytes<0){
                    fprintf(stderr, " before break recive ack num is 0x%x\n", recived_ih_seq);
                    if(recived_ih_seq==ackseqn) return 0;
                    else return (-1);
                  }
                  recived_ih_seq = ntohl(ihdr_ack.ih_seqn);
                }
            }
    }

  }
  return (-1);
}

/* 
 * sendimsg: send the specifics of the image (width, height, etc.) in
 * an imsg_t packet to the client.  The data type imsg_t is defined in
 * netimg.h.  Assume that other than im_vers, the other fields of
 * imsg_t are already correctly filled, but integers are still in host
 * byte order.  Fill in im_vers and convert integers to network byte
 * order before transmission.  The field im_type is set by the caller
 * and should not be modified.  Call sendto() to send the imsg
 * packet to the client.
 *
 * Upon successful sent, return 0, else return -1. 
 * Nothing else is modified.
*/
int imgdb::
sendimsg(imsg_t *imsg)
{
  imsg->im_vers = NETIMG_VERS;
  imsg->im_width = htons(imsg->im_width);
  imsg->im_height = htons(imsg->im_height);

  return(sendpkt((char *) imsg, sizeof(imsg_t), NETIMG_SYNSEQ, 0));
}

/*
 * sendimg:
 * Send the image contained in *image to the client.  Send the image
 * in chunks of segsize, not to exceed mss, instead of as one single
 * image. With probability pdrop, drop a segment instead of sending
 * it.
 *
 * Terminate process upon encountering any error.
 * Doesn't otherwise modify anything.
*/
void imgdb::
sendimg(char *image, long imgsize)
{
  int bytes, segsize, datasize;
  char *ip;
  long left;
  unsigned int snd_next=0;
  if (!image) {
    return;
  }
  
  ip = image; /* ip points to the start of image byte buffer */
  datasize = mss - sizeof(ihdr_t) - NETIMG_UDPIP;
  
  /* Lab5 Task 1:
   * make sure that the send buffer is of size at least mss.
   */
  int mss_in = (int) mss;
  int err_SO_SNDBUF = setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &mss_in , sizeof(int));
  if(err_SO_SNDBUF==-1) perror("setsockopt SO_SNDBUF failed");


  int recive_buffer_sized;
  socklen_t len_int = sizeof(int);

  int error = getsockopt(sd, SOL_SOCKET, SO_RCVBUF, &recive_buffer_sized, &len_int);
  fprintf(stderr, "the recive buffer is %d\n", recive_buffer_sized);
  /* Lab5 Task 1:
   *
   * Populate a struct msghdr with information of the destination
   * client, a pointer to a struct iovec array.  The iovec array
   * should be of size NETIMG_NUMIOV.  The first entry of the iovec
   * should be initialized to point to an ihdr_t, which should be
   * re-used for each chunk of data to be sent.
   */
  /* Lab5: YOUR CODE HERE */
    struct iovec iov[NETIMG_NUMIOV];

    struct msghdr mh ={&client, sizeof(struct sockaddr_in), iov, NETIMG_NUMIOV, NULL, 0, 0};

    ihdr_t io_header;
    io_header.ih_vers = NETIMG_VERS;
    io_header.ih_type = NETIMG_DATA;
    io_header.ih_seqn = 0;


    iov[0].iov_base  = &io_header;
    iov[0].iov_len = sizeof(io_header);

  /* PA3 Task 2.2: initialize any necessary variables
   * for your sender side sliding window.
   */
  /* PA3: YOUR CODE HERE */
    unsigned int snd_una = 0;
    unsigned int rwnd_short = rwnd*datasize;
    unsigned int useable_window = rwnd_short - (snd_next - snd_una);
    fprintf(stderr, "useable_window%hu mss is%hu\n", useable_window, mss );

  do {
    /* PA3 Task 2.2: estimate the receiver's receive buffer based on packets
     * that have been sent and ACKed, including outstanding FEC packet(s).
     * We can only send as much as the receiver can buffer.
     * It's an estimate, so it doesn't have to be exact, being off by
     * one or two packets is fine.
     */
    /* PA3: YOUR CODE HERE */
    useable_window = rwnd_short - (snd_next - snd_una);

    /* PA3 Task 2.2: Replace the while condition below to send one
     * usable window-full of data to client.  Use sendmsg() to send
     * each segment as you did in Lab5. As in Lab5, probabilistically
     * drop a segment instead of sending it.  Don't forget to
     * decrement your "usable" window when a packet is so dropped.
     */
    while (useable_window>datasize) {
    /* PA3: YOUR CODE HERE */
      //fprintf(stderr, "start to send image");

      // The last segment may be smaller than datasize 
      left = imgsize - snd_next;
      segsize = datasize > left ? left : datasize;
      if(left == 0) break;
      /* probabilistically drop a segment */
      if (((float) random())/INT_MAX < pdrop) {
        fprintf(stderr, "imgdb::sendimg: DROPPED offset 0x%x, %d bytes\n",
                snd_next, segsize);
      } else { 
        
        /* Lab5 Task 1: 
         * Send one segment of data of size segsize at each iteration.
         * Point the second entry of the iovec to the correct offset
         * from the start of the image.  Update the sequence number
         * and size fields of the ihdr_t header to reflect the byte
         * offset and size of the current chunk of data.  Send
         * the segment off by calling sendmsg().
         */
        /* Lab5: YOUR CODE HERE */
        iov[1].iov_base = ip + snd_next;
        iov[1].iov_len = segsize;
        io_header.ih_size = htons(segsize); 
        io_header.ih_seqn = htonl(snd_next);

        int rc = sendmsg(sd, &mh, 0);  
        if (rc == -1) {
          perror("sendmsg xxx failed");
          return;
        }


        fprintf(stderr, "imgdb::sendimg: sent offset 0x%x, %d bytes, unacked: 0x%x\n",
                snd_next, segsize, snd_una);
        
      }

      // PA3 Task 2.2: decrement "usable" window by segment sent (even if dropped)
      /* PA3: YOUR CODE HERE */
      snd_next += segsize;
      useable_window = rwnd_short - (snd_next - snd_una);
    }
    /* PA3 Task 2.2: If an ACK arrived, grab it off the network and
     * slide our window forward when possible. Continue to do so
     * opportunistically for all the ACKs that have arrived.  Remember
     * that we're using cumulative ACK.
     *
     * If no ACK returned up to the timeout time, trigger Go-Back-N
     * and re-send all segments starting from the last unACKed
     * segment.
     */
    /* PA3: YOUR CODE HERE */
    FD_ZERO(&imgdb_fd_set);
    FD_SET(sd, &imgdb_fd_set);
    struct timeval timeout_1;
    timeout_1.tv_sec = NETIMG_SLEEP;
    timeout_1.tv_usec = NETIMG_USLEEP;

    while(1){
      int err = select(sd+1, &imgdb_fd_set, 0, 0, &timeout_1);
      if(!err){// start go back n
        fprintf(stderr, " RTO current unacked is: 0x%x\n", snd_una);
        snd_next = snd_una;
        break;
      }else{
        if(FD_ISSET(sd, &imgdb_fd_set)){
            ihdr_t ihdr_ack;
            socklen_t client_size = sizeof(client);
            while(1){
              int byte_r = recvfrom(sd, &ihdr_ack, sizeof(ihdr_t), MSG_DONTWAIT, (struct sockaddr *) &client, &client_size);
              if(byte_r<0) break;
              if(ihdr_ack.ih_type == NETIMG_ACK){
                unsigned int seq_short =  ntohl(ihdr_ack.ih_seqn);
                snd_una = max(snd_una, seq_short);
                fprintf(stderr, " server finish recive ack 0x%x current unacked is: 0x%x\n", seq_short, snd_una);
              }else{
                fprintf(stderr, "!!! recived type is not net ack");
              }
            }
            snd_next = snd_una;
        }
      }
    }
    //fprintf(stderr, "current snd_next is  %d, and imgsize is %li\n", snd_next, imgsize);

  } while ((int)snd_next < imgsize); // PA3 Task 2.2: replace the '1' with your condition for detecting 
  // that all segments sent have been acknowledged
    
  /* PA3 Task 2.2: after the image is sent send a NETIMG_FIN packet
   * and wait for ACK, using imgdb::recvack().
   */ 
  ihdr_t hdr;
  hdr.ih_vers = NETIMG_VERS;
  hdr.ih_type = NETIMG_FIN;
  hdr.ih_size = 0;
  hdr.ih_seqn = htonl(NETIMG_FINSEQ);

  fprintf(stderr, "imgdb::sendimg: send FIN, unacked: 0x%x\n", snd_una);
  if (!sendpkt((char *) &hdr, sizeof(ihdr_t), NETIMG_FINSEQ, 1)) {
    fprintf(stderr, "imgdb::sendimg: FIN acked.\n");
  }

  return;
}

/*
 * handleqry: receives a query packet, searches
 * for the queried image, and replies to client.
 */
void imgdb::
handleqry()
{
  iqry_t iqry;
  imsg_t imsg;
  double imgsize_d;

  imsg.im_type = recvqry(&iqry);
  if (imsg.im_type) {
    fprintf(stderr, "imgdb::handleqry: recvqry returns 0x%x.\n", imsg.im_type);
    sendimsg(&imsg);
  } else {
    
    imsg.im_type = readimg(iqry.iq_name, 1);
    
    if (imsg.im_type == NETIMG_FOUND) {
      mss = (unsigned short) ntohs(iqry.iq_mss);
      // Lab6 and PA3:
      rwnd = iqry.iq_rwnd;
      fwnd = iqry.iq_fwnd;

      imgsize_d = marshall_imsg(&imsg);
      net_assert((imgsize_d > (double) LONG_MAX), "imgdb: image too big");
      if (!sendimsg(&imsg)) {  // if imsg sent and ACKed successfully
        sendimg((char *) curimg.GetPixels(), (long)imgsize_d);
      }
    } else {
      sendimsg(&imsg);
    }
  }

  return;
}

int
main(int argc, char *argv[])
{ 
  socks_init();

  imgdb imgdb;
  // parse args, see the comments for imgdb::args()
  if (imgdb.args(argc, argv)) {
    fprintf(stderr, "Usage: %s [ -d <prob> ]\n",
            argv[0]); 
    exit(1);
  }

  while (1) {
    imgdb.handleqry();
  }
    
#ifdef _WIN32
  WSACleanup();
#endif // _WIN32
  
  exit(0);
}
