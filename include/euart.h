#ifndef EUART_H_
#define EUART_H_


#include <driver/uart.h>

#include <uaio.h>


#define EUIF_STDIO  0x1
#define EUIF_NONBLOCK 0x2


#define EUART_DEVICE_FLUSH(d) fsync((d)->fd)
#define EUART_DEVICE_PRINTF(d, ...) dprintf((d)->outfd, __VA_ARGS__)


typedef struct euart_device {
    int no;
    int flags;
    int infd;
    int outfd;
} euart_device_t;


enum euart_taskstatus {
    EUTS_OK,
    EUTS_EOF,
    EUTS_TIMEDOUT,
    EUTS_ERROR,
};


/* abstract */
struct euart_task {
    /* set by user */
    unsigned long timeout_us;
    struct euart_device *device;

    /* set by machine */
    enum euart_taskstatus status;
};


typedef struct euart_reader {
    struct euart_task;

    /* set by user */
    char *buff;
    unsigned int maxbytes;
    unsigned int minbytes;

    /* set by machine */
    int bytes;
} euart_reader_t;


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY euart_reader
#include "uaio_generic.h"


int
euart_device_init(struct euart_device *d, uart_config_t *config, int no, \
        int txpin, int rxpin, int flags);


ASYNC
euart_readA(struct uaio_task *self, struct euart_reader *r);


#define EUART_AREAD(task, reader) \
    UAIO_AWAIT(task, euart_reader, euart_readA, reader)


// int
// euart_write(struct uaio_task *self
#endif  // EUART_H
