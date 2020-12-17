#ifndef COMMON_H
#define COMMON_H

#include <sys/time.h>

#define NETWORK_EMULATOR_PORT       50001
#define TRANSMITTER_PORT            50000
#define RECEIVER_PORT               50002
#define PAYLOAD_LEN                 256
#define INITIAL_WINDOW_SIZE         1
#define MAX_WINDOW_SIZE             20
#define INITIAL_SEQ_NUM             1

#define TRANSMITTER_IP                  "127.0.0.1"
#define NETWORK_EMULATOR_IP             "127.0.0.1"
#define RECEIVER_IP                     "127.0.0.1"

long delay(struct timeval t1, struct timeval t2)
{
    long d;

    d = (t2.tv_sec - t1.tv_sec) * 1000;
    d += ((t2.tv_usec - t1.tv_usec) / 1000);
    return(d);
}

#endif
