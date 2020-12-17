#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "packet.h"

enum LogType { DEBUG, INFO, ERROR };
#define DEFAULT_LOGGER_LEVEL INFO

#define LOG_FILE_DIR           "./logs"
#define LOG_FILE_PATH          "./logs/out.log"

void logToFile(enum LogType severity, struct packet* pkt, const char* format, ...)
{
    time_t rawtime;
    struct tm* tinfo;
    char tbuffer[24];
    char* msg;
    va_list args;
    size_t len;

    struct stat st = {};

    if (stat("./logs", &st) == -1)
    {
        #if defined(_WIN32)
            mkdir(LOG_FILE_DIR);
        #else
            mkdir(LOG_FILE_DIR, 0777);
        #endif
    }

    FILE* fptr = fopen(LOG_FILE_PATH, "a");
    if (fptr == NULL)
    {
        perror("could not open log file");
        return;
    }

    // allocate memory for string
    va_start(args, format);
    len = vsnprintf(0, 0, format, args);
    va_end(args);

    if ((msg = (char *)malloc(len + 1)) != NULL)
    {
        va_start(args, format);
        vsnprintf(msg, len + 1, format, args);
        va_end(args);
    }

    if (msg)
    {
        time(&rawtime);
        tinfo = localtime(&rawtime);
        sprintf(tbuffer, "%d-%d-%d %d:%d:%d", tinfo->tm_year + 1900, tinfo->tm_mon + 1, tinfo->tm_mday, tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);

        switch (severity)
        {
            case DEBUG:
                if (DEFAULT_LOGGER_LEVEL == DEBUG)
                {
                    fprintf(stdout, "[%s] %s\n", tbuffer, msg);
                    fprintf(fptr, "[DEBUG][%s] %s\n", tbuffer, msg);
                }
                break;
            case INFO:
                fprintf(stdout, "[%s] %s\n", tbuffer, msg);
                fprintf(fptr, "[INFO][%s] %s\n", tbuffer, msg);
                break;
            case ERROR:
                fprintf(stderr, "[%s] %s\n", tbuffer, msg);
                fprintf(fptr, "[ERROR][%s] %s\n", tbuffer, msg);
                break;
            default:
                fprintf(stderr, "invalid severity level\n");
        }

        if (pkt != NULL)
        {
            char* data = (char *)malloc(strlen(pkt->data) + 1);
            char* packetType = packetTypeToString(pkt->packetType, false);
            char* retransmit = retransmitToString(pkt->retransmit);
            strcpy(data, pkt->data);
            fprintf(fptr, "{\n    packetType: %s,\n    seqNum: %i,\n    data: %s,\n    windowSize: %i,\n    ackNum: %i,\n    retransmit: %s,\n}\n",
                packetType, pkt->seqNum, strtok(data, "\n"), pkt->windowSize, pkt->ackNum, retransmit
            );
            free(data);
            free(packetType);
            free(retransmit);
        }

        free(msg);
    }
    else
    {
        fprintf(fptr, "[ERROR][%s] error while logging a message: memory allocation failed.\n", tbuffer);
    }
    fclose(fptr);
}

#endif
