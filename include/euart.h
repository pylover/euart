#ifndef EUART_H_
#define EUART_H_


#include <caio/caio.h>


#define CUIF_STDIO  0x1


#define EUART_FLUSH(c) fsync((c)->fd)


typedef struct euart {
    int fd;
} euart_t;


enum euart_chatstatus {
    CUCS_OK,
    CUCS_TIMEDOUT,
    CUCS_ERROR,
};


enum euart_chatflags {
    CUCF_DROPEMPTYLINES = 1,
};


struct euart_chat {
    /* set by user */
    unsigned long timeout_us;
    int flags;
    void *userptr;
    union {
        int count;
    } query;

    /* set by machine */
    enum euart_chatstatus status;
    union {
        char uint8;
        char *str;
    } result;
};


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY euart
#define CAIO_ARG1 struct euart_chat *
#include "caio/generic.h"


#define EUART_PRINTF(u, ...) dprintf((u)->fd, __VA_ARGS__)


int
euart_init(struct euart *u, int no, int txpin, int rxpin, int flags);


ASYNC
euart_getc(struct caio_task *self, struct euart *u, struct euart_chat *q);


ASYNC
euart_readA(struct caio_task *self, struct euart *u, struct euart_chat *c);


#endif  // EUART_H_
