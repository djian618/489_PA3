#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "transdef.h"                     /* application header file  */

/* This routine writes banking transactions to one of several         */
/* regional servers (chosen based on the contents of the transaction, */
/* not based on the location of the local host). "s" is a datagram    */
/* socket. "head" and "trail" are application level transaction       */
/* header and trailer components. "trans" is the body of the          */
/* transaction. Reliability must be ensured by the caller (because    */
/* datagrams do not provide it). The server receives all three        */
/* parts as a single datagram.                                        */
puttrans(int s, struct header *head, struct record *trans,
         struct trailer *trail)
{
   int rc;

      /* socket address for server                                    */
   struct sockaddr_in dest;

      /* will contain information about the remote host               */
   struct hostent *host;
   char fullname[64];

      /* Will point to the segments of the (noncontiguous)            */
      /* outgoing message.                                            */
   struct iovec iov[3];

      /* This structure contains parameter information for sendmsg.   */
   struct msghdr mh;
      /* Choose destination host from region field of the data        */
      /* header. Then find its IP address.                            */
   strcpy(fullname,head->region);
   strcat(fullname,".mis.bank1.com");
   host = gethostbyname(fullname);
   if (host==NULL) {
       printf("Host %s is unknown.\n",fullname);
       return -1;
   }

   /* Fill in socket address for the server. We assume a        */
   /* standard port is used.                                                */
   memset(&dest,'\0',sizeof(dest));
   dest.sin_family = AF_INET;
   memcpy(&dest.sin_addr,host->h_addr,sizeof(dest.sin_addr));
   dest.sin_port = htons(TRANSACTION_SERVER);

      /* Specify the components of the message in an "iovec".   */
   iov[0] .iov_base = (caddr_t)head;
   iov[0] .iov_len = sizeof(struct header);
   iov[1] .iov_base = (caddr_t)trans;
   iov[1] .iov_len = sizeof(struct record);
   iov[2] .iov_base = (caddr_t)trail;
   iov[2] .iov_len = sizeof(struct trailer);

      /* The message header contains parameters for sendmsg.    */
   mh.msg_name = (caddr_t) &dest;
   mh.msg_namelen = sizeof(dest);
   mh.msg_iov = iov;
   mh.msg_iovlen = 3;
   mh.msg_accrights = NULL;            /* irrelevant to AF_INET */
   mh.msg_accrightslen = 0;            /* irrelevant to AF_INET */

   rc = sendmsg(s, &mh, 0);            /* no flags used         */
   if (rc == -1) {
      perror("sendmsg failed");
      return -1;
   }
   return 0;
}
