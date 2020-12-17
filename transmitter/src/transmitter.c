#include "../../common.h"
#include "../../logger.h"
#include "transmitter.h"

int main(int argc, char** argv)
{
    const char* const programName = argv[0];
    const char* host = NULL;
    char const* fileName = DATA_FILE_PATH;

    FILE* dataFP = NULL;

    int	port = NETWORK_EMULATOR_PORT;
    int windowSize = INITIAL_WINDOW_SIZE, seqNum = INITIAL_SEQ_NUM, packetSize = sizeof(struct packet);
    int timeoutInterval = DEFAULT_ESTIMATED_RTT + 4 * DEFAULT_DEV_RTT, estimatedRTT = DEFAULT_ESTIMATED_RTT, devRTT = DEFAULT_DEV_RTT, sampleRTT = 0;
    int	socketFileDescriptor =	0;

    struct node* unACKHead = NULL;

    struct hostent* hp;
    struct sockaddr_in receiver, transmitter;
    struct timeval start, end, readTimeout;
    readTimeout.tv_sec = 0;
    readTimeout.tv_usec = DEFAULT_READ_TIMEOUT;

    struct packet arrPackets[MAX_READ_SIZE];
    struct packet* arrPacketsPtr;
    arrPacketsPtr = &arrPackets[0];

    struct packet* ACKPacketPtr = malloc(packetSize);

    socklen_t receiverLen;

    // get user parameters
    switch (argc)
    {
        case 1: // user doesn't specify any arguments
            // get receiver IP using config file
            host = NETWORK_EMULATOR_IP;
            if ((hp = gethostbyname(host)) == NULL)
            {
                logToFile(ERROR, NULL, "Unknown server address: %s", host);
                exit(1);
            }
            logToFile(INFO, NULL, "Host found: %s", host);
            break;
        case 2: // user specifies one argument
            // get receiver IP either using FQDN or IP address
            host = argv[1];
            if ((hp = gethostbyname(host)) == NULL)
            {
                logToFile(ERROR, NULL, "Unknown server address: %s", host);
                exit(1);
            }
            logToFile(INFO, NULL, "Host found: %s", host);
            break;
        case 3: // user specifies two arguments
            // get receiver IP either using FQDN or IP address
            host = argv[1];
            if ((hp = gethostbyname(host)) == NULL)
            {
                logToFile(ERROR, NULL, "Unknown server address: %s", host);
                exit(1);
            }
            logToFile(INFO, NULL, "Host found: %s", host);

            // verify file is valid
            dataFP = fopen(argv[2], "r");
            if (dataFP != NULL)
            {
                fclose(dataFP);
                fileName = argv[2];
            }
            else
            {
                logToFile(ERROR, NULL, "File: %s could not be opened", argv[2]);
                exit(1);
            }
            break;
        default:
            logToFile(ERROR, NULL, "Usage: %s [hostName] [fileName]", programName);
            exit(1);
    }

    // create a datagram socket
    if ((socketFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        logToFile(ERROR, NULL, "Can't create a socket\n");
        exit(1);
    }

    // store receiver's information
    bzero((char*)&receiver, sizeof(receiver));
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port);
    receiverLen = sizeof(receiver);

    logToFile(INFO, NULL, "The network emulator's port is: %d", port);
    bcopy(hp->h_addr, (char*)&receiver.sin_addr, hp->h_length);

    // set socket options
    if (setsockopt(socketFileDescriptor, SOL_SOCKET, SO_RCVTIMEO, &readTimeout, sizeof(readTimeout)) < 0)
    {
        logToFile(ERROR, NULL, "setsockopt failed");
        exit(1);
    }

    // bind local address to the socket on transmitter
    bzero((char*)&transmitter, sizeof(transmitter));
    transmitter.sin_family = AF_INET;
    transmitter.sin_port = htons(TRANSMITTER_PORT);
    transmitter.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketFileDescriptor, (struct sockaddr*)&transmitter, sizeof(transmitter)) == -1)
    {
        logToFile(ERROR, NULL, "Can't bind name to socket");
        exit(1);
    }

    logToFile(INFO, NULL, "Sending data in file path: %s", DATA_FILE_PATH);
    if (sizeof(arrPackets) > MAX_BUF_LEN)
    {
        logToFile(ERROR, NULL, "Loaded Data is larger than buffer size");
        exit(1);
    }

    // store all the file data in an array of structs
    int lineCounter = 0;
    int totalLines = 0;

    dataFP = fopen(fileName, "r");
    while (fgets(arrPackets[lineCounter].data, PAYLOAD_LEN, dataFP))
    {
        lineCounter++;
    }

    totalLines = lineCounter;
    lineCounter = 0;

    logToFile(INFO, NULL, "number of lines in the file are: %d", totalLines);

    // send a window of packets and wait for ACKs before creating new window
    enum State state = SendingPackets;
    while (state != AllPacketsSent)
    {
        switch (state)
        {
            case SendingPackets:
                logToFile(INFO, NULL, "current window size: %d", windowSize);
                // create a window of packets to send and transmit datagrams to the receiver
                for (int windowCounter = 0; windowCounter < windowSize; ++windowCounter, lineCounter++)
                {
                    // generate a list of unACK packets containing sequence numbers
                    appendToUnACKs(&unACKHead, seqNum);

                    // initialize remaining packet fields
                    arrPackets[lineCounter].packetType = DATA;
                    arrPackets[lineCounter].seqNum = seqNum++;
                    arrPackets[lineCounter].windowSize = windowSize;
                    arrPackets[lineCounter].ackNum = INVALID_ACK_NUM;
                    arrPackets[lineCounter].retransmit = false;

                    // send to receiver
                    if (sendto(socketFileDescriptor, arrPacketsPtr++, packetSize, 0, (struct sockaddr*)&receiver, receiverLen) == -1)
                    {
                        logToFile(ERROR, NULL, "sendto failure");
                        exit(1);
                    }
                    logToFile(INFO, arrPacketsPtr-1, "sent DATA (seqNum: %d)", arrPackets[lineCounter].seqNum);
                    
                    // if last data packet is sent, update line counter immediately, stop sending and immediately wait for ACKs
                    if (lineCounter + 1 == totalLines)
                    {
                        lineCounter += 1;
                        state = WaitForACKs;
                        gettimeofday(&start, NULL);
                        break;
                    }
                }

                // start delay measure for timeout events
                gettimeofday(&start, NULL);

                logToFile(INFO, NULL, "window of packets sent, waiting for ACKs");
                state = WaitForACKs;
                break;
            case WaitForACKs:
                // check if all ACKs have been received
                if (getUnACKCount(unACKHead) == 0)
                {
                    logToFile(INFO, NULL, "all ACKs received\n");
                    state = AllACKsReceived;
                    break;
                }
                
                // check for timeout events, with end delay measure 
                gettimeofday(&end, NULL);
                logToFile(DEBUG, NULL, "current delay = %ld ms.\n", delay(start, end));

                if (delay(start, end) >= timeoutInterval && getUnACKCount(unACKHead)> 0)
                {
                    logToFile(INFO, NULL, "RTT (%ld) >= Timeout Interval (=%d), packet loss event detected", delay(start, end), timeoutInterval);
                    logToFile(INFO, NULL, "retransmitting %d unACKs...", getUnACKCount(unACKHead));
                    if (DEFAULT_LOGGER_LEVEL == DEBUG) printUnACKs(unACKHead);

                    // retransmit unACKed packets
                    retransmitUnACKs(socketFileDescriptor, arrPackets, unACKHead, packetSize, &receiver, receiverLen);

                    // update Timeout Interval based
                    updateTimeoutInterval(&timeoutInterval, &sampleRTT, &start, &end, &estimatedRTT, &devRTT);

                    // reset delay measure for timeout events
                    gettimeofday(&start, NULL);

                    // reduce window size by half
                    windowSize /= 2;
                }

                // receive data from the receiver (non-blocking)
                if (recvfrom(socketFileDescriptor, ACKPacketPtr, packetSize, 0, (struct sockaddr*)&receiver, &receiverLen) >= 0)
                {
                    logToFile(DEBUG, NULL, "size of unACKs list: %d", getUnACKCount(unACKHead));
                    logToFile(INFO, ACKPacketPtr, "received ACK (ackNum: %d)", ACKPacketPtr->ackNum);

                    // update Timeout Interval based on sampleRTT
                    updateTimeoutInterval(&timeoutInterval, &sampleRTT, &start, &end, &estimatedRTT, &devRTT);

                    // check to see if data from receiver contains ACK we haven't received yet
                    struct node* current = unACKHead;
                    while (current != NULL)
                    {
                        // matching ACK found
                        if (current->data == ACKPacketPtr->ackNum)
                        {
                            logToFile(DEBUG, NULL, "ACK found: %d, removing now...", ACKPacketPtr->ackNum);
                            deleteFromUnACKs(&unACKHead, ACKPacketPtr->ackNum);
                            if (DEFAULT_LOGGER_LEVEL == DEBUG) printUnACKs(unACKHead);

                            // increase window size by one
                            if(windowSize!=MAX_WINDOW_SIZE)	windowSize++;
                            break;
                        }
                        // no match found, continue to next node
                        current = current->next;
                    }
                }
                break;
            case AllACKsReceived:
                logToFile(DEBUG, NULL, "line Counter %d", lineCounter);
                logToFile(DEBUG, NULL, "total lines %d", totalLines);
                state = (lineCounter == totalLines) ? AllPacketsSent : SendingPackets;
                freeUnACKs(&unACKHead);
                break;
            default:
                logToFile(ERROR, NULL, "unknown state: %d", state);
                freeUnACKs(&unACKHead);
                free(ACKPacketPtr);
                exit(1);
        }
    }

    logToFile(INFO, NULL, "completed data transfer");
    logToFile(INFO, NULL, "sending EOT Packet");
    struct packet* EOTPacket = malloc(packetSize);
    makePacket(EOTPacket, EOT);

    // ensure EOT delivery
    for (int i = 0; i < 10; ++i) 
    {
        // send EOT to receiver 
        if (sendto(socketFileDescriptor, EOTPacket, packetSize, 0, (struct sockaddr*)&receiver, receiverLen) == -1)
        {
            logToFile(ERROR, NULL, "sendto failure");
            exit(1);
        }
    }

    logToFile(INFO, NULL, "terminating transmitter...");

    free(EOTPacket);
    freeUnACKs(&unACKHead);
    free(ACKPacketPtr);
    close(socketFileDescriptor);
    return(0);
}

void updateTimeoutInterval(int* timeoutInterval, int* sampleRTT, struct timeval* start, struct timeval* end, int* estimatedRTT, int* devRTT)
{
    *sampleRTT = delay(*start, *end);
    *estimatedRTT = (1 - DEFAULT_RTT_ALPHA) * (*estimatedRTT) + DEFAULT_RTT_ALPHA * (*sampleRTT);
    *devRTT = (1 - DEFAULT_RTT_BETA) * (*devRTT) + DEFAULT_RTT_BETA * abs(*sampleRTT - *estimatedRTT);
    *timeoutInterval = (MAX_TIMEOUT_INTERVAL > (*estimatedRTT + 4 * (*devRTT))) ? *estimatedRTT + 4 * (*devRTT) : MAX_TIMEOUT_INTERVAL;
    logToFile(INFO, NULL, "updating timeout interval: %d", *timeoutInterval);
}

void appendToUnACKs(struct node** headRef, int seqNum)
{
    // allocate node
    struct node* newNode = (struct node*)malloc(sizeof(struct node));
    struct node* last = *headRef;

    // put in data
    newNode->data = seqNum;
    // new node is going to be last node
    newNode->next = NULL;
    // if linked list is empty, make new node as head
    if (*headRef == NULL)
    {
        *headRef = newNode;
        return;
    }
    // else traverse till the last node
    while (last->next != NULL)
    {
        last = last->next;
    }
    // change next of last node
    last->next = newNode;
    return;
}

void deleteFromUnACKs(struct node** headRef, int seqNum)
{
    // store head node
    struct node* temp = *headRef, * prev = NULL;

    // If head node holds seqNum to be deleted
    if (temp != NULL && temp->data == seqNum)
    {
        *headRef = temp->next;
        free(temp);
        temp = NULL;
    }

    // search for seqNum to be deleted, keeping track of previous node
    while (temp != NULL && temp->data != seqNum)
    {
        prev = temp;
        temp = temp->next;
    }

    // if seqNum not present in linked list
    if (temp == NULL) return;

    // unlink node from linked list
    prev->next = temp->next;
    free(temp);
}

// get number of nodes in linked list
int getUnACKCount(struct node* head)
{
    int count = 0;
    struct node* current = head;
    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    return count;
}

void freeUnACKs(struct node** headRef)
{
    // deref headRef to get real head
    struct node* current = *headRef;
    struct node* next;

    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    // deref headRef to affect the real head back in caller
    *headRef = NULL;
}

void printUnACKs(struct node* node)
{
    printf("sequence nums: ");
    while (node != NULL)
    {
        printf(" %d ", node->data);
        node = node->next;
    }
    printf("\n");
}

void retransmitUnACKs(int socketFileDescriptor, struct packet* arrPackets, struct node* head, int packetSize, struct sockaddr_in* receiver, socklen_t receiverLen)
{
    struct node* current = head;
    while (current != NULL)
    {
        struct packet* arrPacketsPtr;
        arrPacketsPtr = &arrPackets[current->data - 1];
        arrPacketsPtr->retransmit = true;
        if (sendto(socketFileDescriptor, arrPacketsPtr, packetSize, 0, (struct sockaddr*)receiver, receiverLen) == -1)
        {
            perror("sendto retransmit failure");
            exit(1);
        }
        current = current->next;
    }
}