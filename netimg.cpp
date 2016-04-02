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
#include <stdlib.h>        // atoi()
#include <assert.h>        // assert()
#include <limits.h>        // LONG_MAX
#include <math.h>          // ceil()
#include <errno.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>      // socklen_t
#include "wingetopt.h"
#else
#include <string.h>        // memset(), memcmp(), strlen(), strcpy(), memcpy()
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API
#include <sys/ioctl.h>     // ioctl(), FIONBIO
#endif
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "netimg.h"
#include "socks.h"
#include "fec.h"

long img_size;
unsigned char *image;
netimg netimg;
/*
 * args: parses command line args.
 *
 * Returns 0 on success or 1 on failure.  On successful return,
 * "*sname" points to the server's name, and "port" points to the port
 * to connect at server, in network byte order.  Both "*sname", and
 * "port" must be allocated by caller.  The variable "*imgname" points
 * to the name of the image to search for. The imgdb member variables
 * mss, rwnd, and fwnd are initialized.
 *
 * Nothing else is modified.
 */
int netimg::
args(int argc, char *argv[], char **sname, unsigned short *port, char **imgname)
{
  char c, *p;
  extern char *optarg;
  int arg;

  if (argc < 5) {
    return (1);
  }
  
  rwnd = NETIMG_RCVWIN;
  mss = NETIMG_MSS;

  while ((c = getopt(argc, argv, "s:q:m:w:d:")) != EOF) {
    switch (c) {
    case 's':
      for (p = optarg+strlen(optarg)-1;  // point to last character of
                                         // addr:port arg
           p != optarg && *p != NETIMG_PORTSEP;
                                         // search for ':' separating
                                         // addr from port
           p--);
      net_assert((p == optarg), "netimg::args: server address malformed");
      *p++ = '\0';
      *port = htons((u_short) atoi(p)); // always stored in network byte order

      net_assert((p-optarg > NETIMG_MAXFNAME),
                 "netimg::args: server's name too long");
      *sname = optarg;
      break;
    case 'q':
      net_assert((strlen(optarg) >= NETIMG_MAXFNAME),
                 "netimg::args: image name too long");
      *imgname = optarg;
      break;
    case 'm':
      arg = atoi(optarg);
      if (arg < NETIMG_MINSS) {
        return(1);
      }
      mss = (unsigned short) arg;
      break;
    case 'w':
      arg = atoi(optarg);
      if (arg < 1 || arg > NETIMG_MAXWIN) {
        return(1);
      }
      rwnd = (unsigned char) arg; 
      break;
    case 'd':
      pdrop = atof(optarg);  // global
      if (pdrop > 0.0 && (pdrop > NETIMG_MAXPROB || pdrop < NETIMG_MINPROB)) {
        fprintf(stderr, "%s: recommended drop probability between %f and %f.\n",
                argv[0], NETIMG_MINPROB, NETIMG_MAXPROB);
      }
      break;
    default:
      return(1);
      break;
    }
  }

  fwnd = NETIMG_FECWIN >= rwnd ? rwnd-1 : NETIMG_FECWIN;  
                                 // used in Lab6 and PA3
  
  return (0);
}

/*
 * sendqry: send a query for provided imgname to
 * connected server.  Query is of type iqry_t, defined in netimg.h.
 * The query packet must be of version NETIMG_VERS and of type
 * NETIMG_SYNQRY both also defined in netimg.h. In addition to the
 * filename of the image the client is searching for, the query
 * message also carries the receiver's window size (rwnd), maximum
 * segment size (mss), and FEC window size (used in Lab6 and PA3).
 * All three are global variables.
 *
 * On send error, return 0, else return 1
 */
int netimg::
sendqry(char *imgname)
{
  int bytes;
  iqry_t iqry;

  iqry.iq_vers = NETIMG_VERS;
  iqry.iq_type = NETIMG_SYNQRY;
  iqry.iq_mss = htons(mss);
  iqry.iq_rwnd = rwnd;
  iqry.iq_fwnd = fwnd;             // used in Lab6 and PA3
  strcpy(iqry.iq_name, imgname); 
  bytes = send(sd, (char *) &iqry, sizeof(iqry_t), 0);
  if (bytes != sizeof(iqry_t)) {
    return(0);
  }

  return(1);
}

void netimg::
reconstruct_image(unsigned char* fec_data){
        unsigned int next_seqn_fec = window_start;
        fprintf(stderr, "reconstruct IMAGE at offst 0x%x \n", next_seqn);
         unsigned int window_end = window_start + (fwnd*datasize)>=img_size? img_size : window_start + (fwnd*datasize);

        while(next_seqn_fec<window_end){
          unsigned int segsize = (img_size - next_seqn_fec < datasize) ? img_size-next_seqn_fec : datasize;
          if(next_seqn != next_seqn_fec){
            fec_accum(fec_data, (image+next_seqn_fec), datasize, segsize);
          }
          next_seqn_fec = next_seqn_fec + segsize;
        }
        unsigned int size_to_be_copy = next_seqn+datasize>img_size? (img_size-next_seqn) : datasize;
        memcpy(image+next_seqn,fec_data, size_to_be_copy);
        delete[] fec_data; 
}




void netimg::
send_ack(ihdr_t* ack){

  /* PA3 Task 2.3:
   * If we're to send back an ACK, send it now.  Probabilistically
   * drop the ACK instead of sending it back (see how this is done in
   * imgdb.cpp).
   */ 
  /* PA3: YOUR CODE HERE */
   
    if(((float) random())/INT_MAX < pdrop) {
        fprintf(stderr, "netimg::send ack: DROPPED with next expected 0x%x\n",
                  next_seqn);
    }else{

        int bytes = send(sd, ack, sizeof(ihdr_t), 0);
        net_assert((bytes<0), "ack sent errror");
        fprintf(stderr, "netimg::send ack: 0x%x\n",
                  ntohl(ack->ih_seqn));
    } 
}
/*
 * recvimsg: receive an imsg_t packet from server and store it
 * in the global variable imsg.  The type imsg_t is defined in
 * netimg.h. Return NETIMG_EVERS if packet is of the wrong version.
 * Return NETIMG_ESIZE if packet received is of the wrong size.
 * Otherwise return the content of the im_type field of the received
 * packet. Upon return, all the integer fields of imsg MUST be in HOST
 * BYTE ORDER. If msg_type is NETIMG_FOUND, compute the size of the
 * incoming image and store the size in the global variable
 * "img_size".
 */
char netimg::
recvimsg()
{
  int bytes;
  double imgsize_d;

  /* receive imsg packet and check its version and type */
  bytes = recv(sd, (char *) &imsg, sizeof(imsg_t), 0); // imsg global
  if (bytes != sizeof(imsg_t)) {
    return(NETIMG_ESIZE);
  }
  if (imsg.im_vers != NETIMG_VERS) {
    return(NETIMG_EVERS);
  }

  if (imsg.im_type == NETIMG_FOUND) {
    imsg.im_height = ntohs(imsg.im_height);
    imsg.im_width = ntohs(imsg.im_width);

    imgsize_d = (double) (imsg.im_height*imsg.im_width*(u_short)imsg.im_depth);
    net_assert((imgsize_d > (double) LONG_MAX), 
               "netimg::recvimsg: image too big");
    img_size = (long) imgsize_d;                 // global

    /* PA3 Task 2.1:
     *
     * Send back an ACK with ih_type = NETIMG_ACK and ih_seqn =
     * NETIMG_SYNSEQ.  Initialize any variable necessary to keep track
     * of ACKs.
     */
    /* PA3: YOUR CODE HERE */
    ihdr_t send_back_ack;
    send_back_ack.ih_vers = NETIMG_VERS;
    send_back_ack.ih_type = NETIMG_ACK;
    send_back_ack.ih_size = htons(sizeof(ihdr_t));
    send_back_ack.ih_seqn = htonl(NETIMG_SYNSEQ);
    //socklen_t client_size = sizeof(client);
    bytes = send(sd, &send_back_ack, sizeof(send_back_ack), 0);
  }

  return((char) imsg.im_type);
}

/* Callback function for GLUT.
 *
 * recvimg: called by GLUT when idle. On each call, receive a
 * chunk of the image from the network and store it in global variable
 * "image" at offset from the start of the buffer as specified in the
 * header of the packet.
 *
 * Terminate process on receive error.
 */
void netimg::
recvimg(void)
{
  ihdr_t hdr;  // memory to hold packet header
  unsigned short format;
   
  /* 
   * Lab5 Task 2:
   * 
   * The image data packet from the server consists of an ihdr_t
   * header followed by a chunk of data.  We want to put the data
   * directly into the buffer pointed to by the global variable
   * "image" without any additional copying. To determine the correct
   * offset from the start of the buffer to put the data into, we
   * first need to retrieve the sequence number stored in the packet
   * header.  Since we're dealing with UDP packet, however, we can't
   * simply read the header off the network, leaving the rest of the
   * packet to be retrieved by subsequent calls to recv(). Instead, we
   * call recv() with flags == MSG_PEEK.  This allows us to retrieve a
   * copy of the header without removing the packet from the receive
   * buffer.
   *
   * Since our socket has been set to non-blocking mode, if there's no
   * packet ready to be retrieved from the socket, the call to recv()
   * will return immediately with return value -1 and the system
   * global variable "errno" set to EAGAIN or EWOULDBLOCK (defined in
   * errno.h).  In which case, this function should simply return to
   * caller.
   * 
   * Once a copy of the header is made to the local variable "hdr",
   * check that it has the correct version number and that it is of
   * type NETIMG_DATA (use bitwise '&' as NETIMG_FEC is also of type
   * NETIMG_DATA).  Terminate process if any error is encountered.
   * Otherwise, convert the size and sequence number in the header
   * to host byte order.
   */
  /* Lab5: YOUR CODE HERE */
    ssize_t head_judge = recv(sd, &hdr, sizeof(hdr), MSG_PEEK);
    if(head_judge == -1){
      //fprintf(stderr, "recv not recive anythign\n");
      return;
    }

    //need to look for hdr.type judgement 
    if (hdr.ih_vers != NETIMG_VERS){
          fprintf(stderr, "wrong  version");
          return;
    }
  /* Lab5 Task 2
   *
   * Populate a struct msghdr with a pointer to a struct iovec
   * array.  The iovec array should be of size NETIMG_NUMIOV.  The
   * first entry of the iovec should be initialized to point to the
   * header above, which should be re-used for each chunk of data
   * received.
   */
  /* Lab5: YOUR CODE HERE */
    struct iovec iov[NETIMG_NUMIOV];
    iov[0].iov_base=&hdr;
    iov[0].iov_len = sizeof(ihdr_t);
    
    struct msghdr message;
    message.msg_name=NULL;
    message.msg_namelen=0;
    message.msg_iov=iov;
    message.msg_iovlen=NETIMG_NUMIOV;
    message.msg_control=0;
    message.msg_controllen=0;

    int h_seqn = ntohl(hdr.ih_seqn);
    int h_size = ntohs(hdr.ih_size);

    //fec_out_rang is the indicator that current seqency is out of windowstart
    datasize = mss - sizeof(ihdr_t) - NETIMG_UDPIP;
    fprintf(stderr, "fwnd is %d\n", fwnd);
    int fec_out_rang  = (h_seqn - window_start)/(fwnd*datasize); 



  /* PA3 Task 2.3: initialize your ACK packet */
    ihdr_t ack_packet;
    ack_packet.ih_vers = NETIMG_VERS;
    ack_packet.ih_type = NETIMG_ACK;
    ack_packet.ih_size = htons(sizeof(ack_packet));

  if (hdr.ih_type == NETIMG_DATA) {
    /* 
     * Lab5 Task 2
     *
     * Now that we have the offset/seqno information from the packet
     * header, point the second entry of the iovec to the correct
     * offset from the start of the image buffer pointed to by the
     * global variable "image".  Both the offset/seqno and the size of
     * the data to be received into the image buffer are recorded in
     * the packet header retrieved above. Receive the segment "for
     * real" (as opposed to "peeking" as we did above) by calling
     * recvmsg().  We'll be overwriting the information in the "hdr"
     * local variable, so remember to convert the size and sequence
     * number in the header to host byte order again.
     */
    /* Lab5: YOUR CODE HERE */
    iov[1].iov_base = image +  ntohl(hdr.ih_seqn);
    iov[1].iov_len = ntohs(hdr.ih_size);
    ssize_t count=recvmsg(sd, &message, 0);
    if(count ==-1){
      fprintf(stderr, "cao xxx can not recive anything");
      close(sd);
      return;
    }
    fprintf(stderr, "netimg::recvimg: received offset 0x%x, %d bytes, waiting for 0x%x\n", 
            h_seqn, h_size, next_seqn);
            
    
    if(go_back_n_mode){
      if(next_seqn == h_seqn){
        //jump out of the go back n mode
        next_seqn = next_seqn+h_size;
        packets_count = 1;
        go_back_n_mode=false;
        window_start = next_seqn;
        fprintf(stderr, "jumped out of goback n at next_seqn 0x%x with size %d\n",next_seqn,h_size);        
     
      }
    }else{  
        //first of all check if the arrived packet is out of range 
        if(fec_out_rang>0){
            if(!go_back_n_mode){
              /*If we have not lost any data segment in the current FEC window, 
              we can simply advance the FEC window to the next window 
              and resume transmission*/
              if(next_seqn == h_seqn){
                window_start = window_start + (fwnd*datasize);
                packets_count = 1;
                next_seqn = next_seqn + h_size;
                fprintf(stderr, "do not recive a fec but could move on next_seqn 0x%x bytes%d \n", next_seqn, h_size);
              }else{
                /*Otherwise, since we've lost the FEC packet, 
                we can't patch any lost segment in the current FEC window, 
                we must enter Go-Back-N mode and wait for the sender to 
                retransmit the lost segment.*/
                go_back_n_mode = true;
                packets_count = 0;
                fprintf(stderr, "do not recive a fec jump to go back n mode\n");
              }
            }
        }else{
              /*If the arriving data segment is the next expected segment (netimg::next_seqn), 
              we increment netimg::next_seqn by the size of the arriving segment. 
              being careful not to count late arriving out-of-order
              packet as belonging to the current FEC window*/
              if(next_seqn == h_seqn){
                next_seqn = next_seqn + h_size;
                fprintf(stderr, "move on next_seqn 0x%x bytes%d \n", next_seqn, h_size);
              }
              packets_count = packets_count + 1;
          }
      }
      ack_packet.ih_seqn = htonl(next_seqn);

      send_ack(&ack_packet);



  }else if(hdr.ih_type == NETIMG_FIN) {  // NETIMG_FIN pkt
      head_judge = recv(sd, &hdr, sizeof(hdr), 0);
    /* PA3 Task 2.3: else it's a NETIMG_FIN packet, prepare to send
       back an ACK with NETIMG_FINSEQ as the sequence number */
    /* PA3 YOUR CODE HERE */ 
      ack_packet.ih_seqn = htonl(NETIMG_FINSEQ);
      send_ack(&ack_packet);

  }



  else if(hdr.ih_type == NETIMG_FEC){
    fprintf(stderr, "!!!!!!!!!!!!  recieve FEC packet at offset 0x%x the packets_count is %d\n", h_seqn, packets_count);
    //recive netimg_fec message
    unsigned char* fec_data = new unsigned char[datasize];; 
    memset(fec_data,0,datasize);
    iov[1].iov_base = fec_data;
    iov[1].iov_len = datasize;
    ssize_t fec_count = recvmsg(sd, &message, 0);
    if(fec_count == -1){
      fprintf(stderr, "can not recive fec_count");
      close(sd);
      return;
    }
    /*If we're not in GBN mode, double check that the FEC window of the
    sender is the same as ours. It is possible that the sender's and 
    receiver's FEC windows have gone out of synch. If the sender's FEC 
    window is ahead of the receiver's (figure out a scenario when this could
    happen and test for it), our count of received packets within the current
    FEC window is most likely meaningless at this point. Since we don't know how
    many segments are lost in the current FEC window, we should treat it as a
    multiple-loss case, enter GBN mode, reset our 
    FEC window to start at netimg::next_seqn and reset any 
    other FEC-related variables.*/
    if(!go_back_n_mode){
     
      // //for the case of the last fwnd window 
      // unsigned int packet_left=0;
      // if(window_end>=img_size){
      //   unsigned int last_packets_size = ((img_size - window_start) % datasize) ? ((img_size - window_start) / datasize + 1) :((img_size - window_start) / datasize);
      //   packet_left = packets_count - (fwnd - last_packets_size);
      //   window_end = img_size;
      // }
      int packets_left = ceil(img_size-window_start)/datasize;
      unsigned int adjust_fwnd = packets_left>fwnd? fwnd:packets_left;
       unsigned int window_end = window_start + (fwnd*datasize)>=img_size? img_size : window_start + (fwnd*datasize);


      if(h_seqn>window_start){
        fprintf(stderr, "OUTSYNECHEED fec go to go back n\n");
        go_back_n_mode = true;

        packets_count = 0;
      }
      else if((packets_count == adjust_fwnd -1)){
        /*If the client has lost at most one segment within the current FEC window, 
        you can re-use your Lab 6 code to reconstruct the lost segment, put it in its 
        appointed place in the image buffer, and send back an ACK acknowledging the full FEC window. 
        If only one segment has been lost in the current FEC window, it is possible that the sender's 
        FEC window is behind that of the receiver's, i.e., the sender's FEC packet has a sequence number 
        smaller than the sequence number that starts the receiver's current FEC window (figure out a scenario
        when this could happen and test for it). In this case, adjust receiver's FEC window to match that of
        the sender's. If the lost segment is still within the adjusted FEC window, you can reconstruct the 
        lost segment as before.*/
        if(h_seqn<window_start){
          if((next_seqn<h_seqn+(fwnd*datasize))&(next_seqn>h_seqn)){
            fprintf(stderr, "adjust window start before reconsturect previous 0x%x now0x%x\n", window_start, h_seqn);
            window_start = h_seqn;
            reconstruct_image(fec_data);
            window_start = window_start + (fwnd*datasize);
           
            next_seqn = window_start;
            ack_packet.ih_seqn = htonl(next_seqn);
            packets_count = 0;
            send_ack(&ack_packet);
          }
        }else if(h_seqn == window_start){
          
          fprintf(stderr, "reconstruct withuot adjust window\n");
          reconstruct_image(fec_data);
          window_start = window_start + (fwnd*datasize);
          packets_count = 0;
         
          next_seqn = window_start;
          ack_packet.ih_seqn = htonl(next_seqn);
          send_ack(&ack_packet);
        }
      }else if(packets_count == fwnd){
          fprintf(stderr, "recive all packets ");
          window_start = window_start + (fwnd*datasize);
          packets_count = 0;
      } else{

          // If more than one segments were lost, in Lab 6 we couldn't
          // recover the lost segments because we didn't have retransmission, 
          // but now that we do have retransmission, we put the client in Go-Back-N mode and 
          // wait for retransmission. Whenever we put the client in GBN mode, we reset the FEC
          // window to start at the next expected sequence number (netimg::next_seqn) and reset 
          // any other FEC-related variables. It takes about 15 lines of code to handle FEC packet,
          // not counting code implemented in Labs 5 and 6 and Task 2 above.
          if(packets_count>adjust_fwnd){
                  fprintf(stderr, "fuck packets_count >>> fwnd which is %d and adjust_fwnd is%d\n", packets_count,adjust_fwnd);
                }
          go_back_n_mode = true;
          window_start = next_seqn;
          packets_count =0;
          fprintf(stderr, "enter go back n mode at 0x%x\n", next_seqn);
      }
  }
  }
  



  /* give the updated image to OpenGL for texturing */
  switch(imsg.im_format) {
  case NETIMG_RGBA:
    format = GL_RGBA;
    break;
  case NETIMG_RGB:
    format = GL_RGB;
    break;
  case NETIMG_GSA:
    format = GL_LUMINANCE_ALPHA;
    break;
  default:
    format = GL_LUMINANCE;
    break;
  }
  
  glTexImage2D(GL_TEXTURE_2D, 0, (GLint) format, (GLsizei) imsg.im_width,
               (GLsizei) imsg.im_height, 0, (GLenum) format, GL_UNSIGNED_BYTE,
               image);

  /* redisplay */
  glutPostRedisplay();

  return;
}

void
recvimg_glut()
{
  netimg.recvimg();

  return;
}

int
main(int argc, char *argv[])
{
  int err;
  char *sname, *imgname;
  u_short port;
  int nonblock=1;

  // parse args, see the comments for netimg::args()
  if (netimg.args(argc, argv, &sname, &port, &imgname)) {
    fprintf(stderr, "Usage: %s -s <server>%c<port> -q <image>.tga [ -w <rwnd [1, 255]> -m <mss (>40)> -d <prob> ]\n", argv[0], NETIMG_PORTSEP); 
    exit(1);
  }

  srandom(NETIMG_SEED);

  socks_init();

  netimg.sd = socks_clntinit(sname, port, netimg.rcvbuf());  // Lab5 Task 2

  if (netimg.sendqry(imgname)) {
    err = netimg.recvimsg();

    if (err == NETIMG_FOUND) { // if image received ok
      netimglut_init(&argc, argv, recvimg_glut);
      netimglut_imginit(netimg.imsg.im_format);
      
      /* set socket non blocking */
      ioctl(netimg.sd, FIONBIO, &nonblock);

      glutMainLoop(); /* start the GLUT main loop */
    } else if (err == NETIMG_NFOUND) {
      fprintf(stderr, "%s: %s image not found.\n", argv[0], imgname);
    } else if (err == NETIMG_EVERS) {
      fprintf(stderr, "%s: wrong version number.\n", argv[0]);
    } else if (err == NETIMG_EBUSY) {
      fprintf(stderr, "%s: image server busy.\n", argv[0]);
    } else if (err == NETIMG_ESIZE) {
      fprintf(stderr, "%s: wrong size.\n", argv[0]);
    } else {
      fprintf(stderr, "%s: image receive error %d.\n", argv[0], err);
    }
  }

  socks_close(netimg.sd); // optional, but since we use connect(), might as well.
  return(0);
}
