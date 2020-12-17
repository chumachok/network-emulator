#include <stdio.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>


enum State { SendingPackets, WaitForACKs, AllACKsReceived, AllPacketsSent };

#define MAX_BUF_LEN             65000   // maximum Buffer length
#define MAX_TIMEOUT_INTERVAL    5000    // maximum Timeout interval in ms
#define DEFAULT_ESTIMATED_RTT   1000    // default estimated round trip time in ms
#define DEFAULT_DEV_RTT         250     // default deviation in round trip time in ms
#define DEFAULT_RTT_ALPHA       0.125   // default constant value used to determine the estimatedRTT
#define DEFAULT_RTT_BETA        0.25    // default constant value used to determine the deviation in sample RTT
#define DEFAULT_READ_TIMEOUT    300     // default recvfrom timeout value in us (prevents indefinite blocking)

#define DATA_FILE_PATH		"./resource/message.txt"

struct node
{
	int data;
	struct node* next;
};

long delay(struct timeval t1, struct timeval t2);
void appendToUnACKs(struct node** headRef, int seqNum);
void deleteFromUnACKs(struct node** headRef, int seqNum);
int getUnACKCount(struct node* head);
void freeUnACKs(struct node** headRef);
void printUnACKs(struct node* node);
void retransmitUnACKs(int socketFileDescriptor, struct packet* arrPackets, struct node* head, int packetSize, struct sockaddr_in* receiver, socklen_t receiverLen);
void updateTimeoutInterval(int* timeoutInterval, int* sampleRTT, struct timeval* start, struct timeval* end, int* estimatedRTT, int* devRTT);