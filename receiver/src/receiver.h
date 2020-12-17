#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

#define OUTPUT_FILE_PATH "./data/message.txt"

void sendACK(int sd, struct packet* pkt, int pktSize, struct sockaddr_in* transmitter, socklen_t transmitterLen);
void saveData(char* data);
void flushBuffer(struct packet* buffer, long long* nextSeqNum, long long* newWindowSeqNum, int windowSize);