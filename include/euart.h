#ifndef EUART_H_
#define EUART_H_


#include <driver/uart.h>

#include <uaio.h>


typedef char u8_t;
#undef ERING_PREFIX
#define ERING_PREFIX u8
#include "ering.h"


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
    struct u8ring ring;

    /* set by user */
    unsigned int minbytes;
} euart_reader_t;


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY euart_reader
#include "uaio_generic.h"


int
euart_device_init(struct euart_device *d, uart_config_t *config, int no, \
        int txpin, int rxpin, int flags);


int
euart_reader_init(struct euart_reader *reader, struct euart_device *dev,
        unsigned int timeout_us, uint8_t ringmaskbits);


int
euart_reader_deinit(struct euart_reader *reader);


ASYNC
euart_readA(struct uaio_task *self, struct euart_reader *r);


#define EUART_AREAD(task, reader) \
    UAIO_AWAIT(task, euart_reader, euart_readA, reader)


#endif  // EUART_H
