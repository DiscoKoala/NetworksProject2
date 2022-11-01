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

    for(;;){
      sendData(sd, server_address, server_address_length);
    }

    close(sd);
    return 0 ;
}

int sendData(int sd, struct sockaddr_in server_address, socklen_t server_address_length){

  int rc = 0;
  char message[BUFFSIZE];
  char tempStr[3];
  char toServer[BUFFSIZE];
  char fromServer[BUFFSIZE];
  int maxLength = 40;
  int messageSize = 0;
  int bytesSent = 0;
  int sequenceNum = 0;
  int ackNum = 0;
  int window = 0;

  time_t timeSent;

  // Clear buffers.
  memset(message, '\0', sizeof(message));
  memset(fromServer, '\0', sizeof(fromServer));
  memset(toServer, '\0', sizeof(toServer));

  // Get string from user.
  printf("Whatchya wanna say?\n");
  char *ptr = fgets(message, maxLength, stdin);

  if (ptr == NULL){
   perror ("fgets");
   return 0;
  }

  // Get length of string, convert length.
  int length = strlen(message);
  int convertedLength = htonl(length);

  // While loop parameter.
  int finalSequenceNum = (sequenceNum + length) - 2;
  printf("\n Final sequence number: %d\n", finalSequenceNum);

  // Send converted length to server.
  rc = sendto(sd, &convertedLength, sizeof(int), 0, (struct sockaddr *) &server_address, sizeof(server_address));

  if(rc < 0){
    perror("sendto");
    return 0;
  }

  int i = 0;
  int j = 0;
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
    printf("\n%s\n", toServer);

    // Send two bytes of data from string to server.
    rc = sendto(sd, toServer, bytesSent, 0, (struct sockaddr *) &server_address, sizeof(server_address));
    timeSent = time(NULL);

    if(rc < 0){
      perror("sendto");
      return 0;
    }

    // Recieve ACK.
    rc = recvfrom(sd, fromServer, sizeof(fromServer), MSG_DONTWAIT, (struct sockaddr *) &server_address, &server_address_length);

    // While waiting for ACK, resend segment if time out occured.
    while(rc < 0){

      if(time(NULL) - timeSent > 1){
        rc = sendto(sd, toServer, bytesSent, 0, (struct sockaddr *) &server_address, sizeof(server_address));
        timeSent = time(NULL);
        printf("\nSegment %d sent\n", sequenceNum);
      }
    }

    // Extract information if ACK is received.
    if(rc > 0){
      sscanf(fromServer, "%11d%4d", &ackNum, &messageSize);
      printf("\nSegment %d ACKed\n", ackNum);
    }

    // If server drops packet(s), go back to last ACKed characters.
    if(ackNum < sequenceNum - 1){
      sequenceNum = ackNum;
      i = ackNum;
   }
   else{
     // Otherwise, send next sequence.
     sequenceNum+=2;
     i+=2;
   }

  }

  return 0;
};
