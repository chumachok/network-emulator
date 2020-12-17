#include "../../common.h"
#include "../../logger.h"
#include "receiver.h"

int main(int argc, char **argv)
{
    int sd, pktSize, latestWindowSize, index;
    long long nextSeqNum, newWindowSeqNum;
    socklen_t transmitterLen;
    struct sockaddr_in receiver, transmitter;

    // create a socket
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        logToFile(ERROR, NULL, "can't create a socket");
        exit(1);
    }

    // bind an address to the socket
    bzero((char *)&receiver, sizeof(receiver));
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(RECEIVER_PORT);
    receiver.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sd, (struct sockaddr *)&receiver, sizeof(receiver)) == -1)
    {
        logToFile(ERROR, NULL, "can't bind name to socket");
        exit(1);
    }

    pktSize = sizeof(struct packet);
    transmitterLen = sizeof(transmitter);
    nextSeqNum = INITIAL_SEQ_NUM;
    newWindowSeqNum = nextSeqNum + INITIAL_WINDOW_SIZE;
    latestWindowSize = INITIAL_WINDOW_SIZE;
    struct packet* packetBuffer = malloc(pktSize * INITIAL_WINDOW_SIZE);
    for (int i = 0; i < INITIAL_WINDOW_SIZE; i++)
        packetBuffer[i].seqNum = INVALID_SEQ_NUM;
    struct packet* pkt = malloc(pktSize);
    while (true)
    {
        if ((recvfrom(sd, pkt, pktSize, 0, (struct sockaddr *)&transmitter, &transmitterLen)) < 0)
        {
            logToFile(ERROR, NULL, "recvfrom error");
            exit(1);
        }
        switch (pkt->packetType)
        {
            case DATA:
                // new window
                if (pkt->seqNum >= newWindowSeqNum)
                {
                    flushBuffer(packetBuffer, &nextSeqNum, &newWindowSeqNum, latestWindowSize);

                    latestWindowSize = pkt->windowSize;
                    newWindowSeqNum = newWindowSeqNum + pkt->windowSize;

                    free(packetBuffer);
                    packetBuffer = malloc(pktSize * pkt->windowSize);
                    for (int i = 0; i < pkt->windowSize; i++)
                        packetBuffer[i].seqNum = INVALID_SEQ_NUM;
                }
                logToFile(INFO, pkt, "received DATA (seqNum: %d)", pkt->seqNum);
                index = pkt->seqNum - newWindowSeqNum + pkt->windowSize;

                // save packet in order
                if (pkt->seqNum == nextSeqNum)
                {
                    saveData(pkt->data);
                    packetBuffer[index].seqNum = INVALID_SEQ_NUM;
                    nextSeqNum++;
                }
                // buffer packet out of order
                else if (pkt->seqNum > nextSeqNum && pkt->seqNum != packetBuffer[index].seqNum)
                {
                    struct packet copyPkt = copyPacket(pkt);
                    packetBuffer[index] = copyPkt;
                }
                sendACK(sd, pkt, pktSize, &transmitter, transmitterLen);
                break;
            case EOT:
                flushBuffer(packetBuffer, &nextSeqNum, &newWindowSeqNum, latestWindowSize);
                logToFile(INFO, pkt, "received EOT packet", pkt);
                logToFile(INFO, pkt, "terminating receiver...", NULL);
                free(packetBuffer);
                close(sd);
                return 0;
                break;
            default:
                logToFile(ERROR, pkt, "received invalid packet, skipping");
        }
    }
    return 0;
}

void flushBuffer(struct packet* buffer, long long *nextSeqNum, long long *newWindowSeqNum, int windowSize)
{
    for (int i = (newWindowSeqNum - nextSeqNum - 1); i < windowSize; i++)
    {
        if (buffer[i].seqNum != INVALID_SEQ_NUM)
        {
            saveData(buffer[i].data);
            buffer[i].seqNum = INVALID_SEQ_NUM;
            nextSeqNum++;
        }
    }
}

void sendACK(int sd, struct packet *pkt, int pktSize, struct sockaddr_in *transmitter, socklen_t transmitterLen)
{
    makePacket(pkt, ACK);
    if (sendto(sd, pkt, pktSize, 0,(struct sockaddr *)transmitter, transmitterLen) != pktSize)
    {
        logToFile(ERROR, NULL, "sendto error");
        exit(1);
    }
    logToFile(INFO, pkt, "sent ACK packet (ackNum: %d)", pkt->ackNum);
}

void saveData(char *data)
{
    FILE *fptr = fopen(OUTPUT_FILE_PATH, "a");
    if (fptr == NULL)
    {
        perror("could not open output file");
        exit(1);
    }
    fprintf(fptr, "%s", data);
    fclose(fptr);
}