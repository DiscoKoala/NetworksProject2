#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFSIZE 100

int sendData(int sd, struct sockaddr_in server_address, socklen_t server_address_length);

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server_address;
    socklen_t server_address_length = sizeof(server_address);
    int portNumber;
    char serverIP[29];

    if (argc < 3){
        printf ("usage is client <ipaddr> <portnumber>\n");
        exit(1);
    }

    // create the socket
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    portNumber = strtol(argv[2], NULL, 10); // convert the port
    strcpy(serverIP, argv[1]); // use the IP address

    server_address.sin_family = AF_INET; //internet addressing
    server_address.sin_port = htons(portNumber); //concert 16 bit port number
    server_address.sin_addr.s_addr = inet_addr(serverIP); // convert ip of server

    sendData(sd, server_address, server_address_length);

    close(sd);
    return 0 ;
}

int sendData(int sd, struct sockaddr_in server_address, socklen_t server_address_length){

  // Unsatisfyingly large amount of variables
  int i = 0;
  int j = 0;
  int rc = 0;
  int windowSize = 10;
  int window = 0;
  int bytesSent = 0;
  int messageSize = 0;
  int ackNum = 0;
  int sequenceNum = 0;
  int totalBytesSent = 0;
  char tempStr[3];
  char message[BUFFSIZE];
  char toServer[BUFFSIZE];
  char fromServer[BUFFSIZE];
  time_t timeSent;

  // Clear buffers.
  memset(message, '\0', sizeof(message));
  memset(fromServer, '\0', sizeof(fromServer));
  memset(toServer, '\0', sizeof(toServer));

  // Get string from user.
  printf("Whatchya wanna say?\n");
  char *ptr = fgets(message, sizeof(message), stdin);

  // Screams at you if something goes wrong.
  if (ptr == NULL){
   perror ("fgets");
   return 0;
  }

  // Get length of string, convert length.
  int length = strlen(message);
  int convertedLength = htonl(length);

  // While loop parameter.
  int finalSequenceNum = (sequenceNum + length) - 2;

  // Send converted length to server.
  rc = sendto(sd, &convertedLength, sizeof(int), 0, (struct sockaddr *) &server_address, sizeof(server_address));

  // Also screams at you.
  if(rc < 0){
    perror("sendto");
    return 0;
  }

  while(ackNum < finalSequenceNum){
    j = 0;
    // If next element is null character, message size is 1.
    if(message[i + 1] == '\0'){
      tempStr[j] = message[i];
      tempStr[j+1] = message[i+1];
      bytesSent = 16;
      messageSize = 1;
    }
    // Otherwise message size is 2.
    else{
      tempStr[j] = message[i];
      tempStr[j+1] = message[i+1];
      tempStr[j+2] = 0;
      bytesSent = 17;
      messageSize = 2;
    }

    // Write to buffer with appropriate header values.
    sprintf(toServer,"%11d%4d%s", sequenceNum, messageSize, tempStr);

    // Send ten bytes rapid fire.
    while(window < windowSize){
      // Send two bytes of data from string to server.
      rc = sendto(sd, toServer, bytesSent, 0, (struct sockaddr *) &server_address, sizeof(server_address));
      timeSent = time(NULL);
      window++;

      // More screaming
      if(rc < 0){
        perror("sendto");
        return 0;
      }
    }

    // Recieve ACK.
    rc = recvfrom(sd, fromServer, sizeof(fromServer), MSG_DONTWAIT, (struct sockaddr *) &server_address, &server_address_length);

    // While waiting for ACK, resend segment if time out occured.
    while(rc < 0){
      if(time(NULL) - timeSent > 2){
        rc = sendto(sd, toServer, bytesSent, 0, (struct sockaddr *) &server_address, sizeof(server_address));
        timeSent = time(NULL);
      }
    }

    // Extract information if ACK is received.
    if(rc > 0){
      // Decrement window variable to signal window shift
      window--;
      sscanf(fromServer, "%11d%4d", &ackNum, &messageSize);
      totalBytesSent += messageSize;
    }

    // If server drops packet(s), go back to last ACKed characters.
    if(ackNum < sequenceNum - 8){
      printf("\nTIMEOUT ******* Resending last ACKed segment.\n");
      sequenceNum = ackNum;
      i = ackNum;
   }
   else{
     // Otherwise, send next sequence.
     sequenceNum+=2;
     i+=2;
   }
 }
 printf("\nString sent was: %s\n", message);
 printf("String length: %d\n", totalBytesSent);
  return 0;
};
