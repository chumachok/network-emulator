#ifndef PACKET_H
#define PACKET_H

#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum PacketType { DATA, ACK, EOT };

#define MAX_READ_SIZE   150
#define INVALID_SEQ_NUM 0
#define INVALID_ACK_NUM 0

#pragma pack(push, 1)
struct packet
{
    enum PacketType packetType;
    int seqNum;
    char data[PAYLOAD_LEN];
    int windowSize;
    int ackNum;
    bool retransmit;
};
#pragma pack(pop)

void makePacket(struct packet* pkt, enum PacketType packetType)
{
    switch (packetType)
    {
        case ACK:
            pkt->packetType = ACK;
            pkt->ackNum = pkt->seqNum;
            pkt->seqNum = INVALID_SEQ_NUM;
            strcpy(pkt->data, "\0");
            pkt->retransmit = false;
            break;
        case EOT:
            pkt->packetType = EOT;
            pkt->ackNum = INVALID_ACK_NUM;
            strcpy(pkt->data, "\0");
            pkt->seqNum = INVALID_SEQ_NUM;
            pkt->retransmit = false;
            break;
        default:
            perror("Not a valid packet type");
            exit(1);
    }
}

struct packet copyPacket(struct packet* pkt)
{
    struct packet copyPkt;

    copyPkt.packetType = pkt->packetType;
    copyPkt.seqNum = pkt->seqNum;
    strcpy(copyPkt.data, pkt->data);
    copyPkt.windowSize = pkt->windowSize;
    copyPkt.ackNum = pkt->seqNum;
    copyPkt.retransmit = pkt->retransmit;

    return copyPkt;
}

char* packetTypeToString(int packetType, bool isDropped)
{
    char* type;
    if(isDropped)
    {
        switch (packetType)
        {
        case DATA:
            type = (char *)malloc(15);
            strcpy(type, (char *)"DATA (DROPPED)");
            break;
        case ACK:
            type = (char *)malloc(14);
            strcpy(type, (char *)"ACK (DROPPED)");
            break;
        case EOT:
            type = (char *)malloc(14);
            strcpy(type, (char *)"EOT (DROPPED)");
            break;
        default:
            type = (char *)malloc(8);
            strcpy(type, (char *)"INVALID");
        }
    }
    else
    {
        switch (packetType)
        {
        case DATA:
            type = (char *)malloc(5);
            strcpy(type, (char *)"DATA");
            break;
        case ACK:
            type = (char *)malloc(4);
            strcpy(type, (char *)"ACK");
            break;
        case EOT:
            type = (char *)malloc(4);
            strcpy(type, (char *)"EOT");
            break;
        default:
            type = (char *)malloc(8);
            strcpy(type, (char *)"INVALID");
        }
    }
    return type;
}

char* retransmitToString(bool retransmit)
{
    char* str;
    if (retransmit)
    {
        str = (char *)malloc(5);
        strcpy(str, (char *)"true");
    }
    else
    {
        str = (char *)malloc(6);
        strcpy(str, (char *)"false");
    }

    return str;
}

#endif
