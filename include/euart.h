#ifndef EUART_H_
#define EUART_H_


#include <uaio.h>


#define EUIF_STDIO  0x1


#define EUART_FLUSH(c) fsync((c)->fd)
#define EUART_PRINTF(u, ...) dprintf((u)->outfd, __VA_ARGS__)


typedef struct euart {
    int infd;
    int outfd;
} euart_t;


enum euart_chatstatus {
    EUCS_OK,
    EUCS_TIMEDOUT,
    EUCS_ERROR,
};


enum euart_chatflags {
    EUCF_DROPEMPTYLINES = 1,
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


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY euart
#define UAIO_ARG1 struct euart_chat *
#include "uaio_generic.h"


int
euart_init(struct euart *u, int no, int txpin, int rxpin, int flags);


ASYNC
euart_getc(struct uaio_task *self, struct euart *u, struct euart_chat *q);


ASYNC
euart_readA(struct uaio_task *self, struct euart *u, struct euart_chat *c);


#endif  // EUART_H_
