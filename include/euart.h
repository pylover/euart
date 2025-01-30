#ifndef EUART_H_
#define EUART_H_


#include <driver/uart.h>

#include <uaio.h>


#define EUIF_STDIO  0x1
#define EUIF_NONBLOCK 0x2


#define EUART_FLUSH(c) fsync((c)->fd)
#define EUART_PRINTF(u, ...) dprintf((u)->outfd, __VA_ARGS__)


typedef struct euart {
    int no;
    int flags;
    int infd;
    int outfd;
} euart_t;


enum euart_querystatus {
    EUCS_OK,
    EUCS_TIMEDOUT,
    EUCS_ERROR,
};


struct euart_query {
    /* set by user */
    unsigned long timeout_us;
    void *backref;

    /* set by machine */
    enum euart_querystatus status;
};


struct euart_getc {
    struct euart_query;
    char c;
};


struct euart_read {
    struct euart_getc;

    /* set by user */
    char *buff;
    int max;

    /* set by machine */
    int bufflen;
};


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY euart
#define UAIO_ARG1 struct euart_query *
#include "uaio_generic.h"


#define EUART_AWAIT(task, coro, state, query) UAIO_AWAIT(task, euart, \
        (euart_coro_t)coro, state, (struct euart_query *)query)


int
euart_init(struct euart *u, uart_config_t *config, int no, int txpin,
        int rxpin, int flags);


ASYNC
euart_getcA(struct uaio_task *self, struct euart *u, struct euart_getc *g);


ASYNC
euart_readA(struct uaio_task *self, struct euart *u, struct euart_read *r);


#endif  // EUART_H_
