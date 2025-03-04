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
    int infd;
    int outfd;
} euart_device_t;


enum euart_taskstatus {
    EUTS_OK,
    EUTS_EOF,
    EUTS_TIMEDOUT,
    EUTS_ERROR,
};


typedef struct euart_reader {
    /* set by user */
    int fd;

    /* set by machine */
    enum euart_taskstatus status;
    struct u8ring ring;
} euart_reader_t;


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY euart_reader
#define UAIO_ARG1 unsigned int
#define UAIO_ARG2 unsigned long
#include "uaio_generic.h"


int
euart_device_init(struct euart_device *d, uart_config_t *config, int no, \
        int txpin, int rxpin, int flags);


int
euart_reader_init(struct euart_reader *reader, int fd, uint8_t ringmaskbits);


int
euart_reader_deinit(struct euart_reader *reader);


ASYNC
euart_readA(struct uaio_task *self, struct euart_reader *r,
        unsigned int minbytes, unsigned long timeout_us);


#define EUART_AREADT(task, reader, minbytes, timeout) \
    UAIO_AWAIT(task, euart_reader, euart_readA, reader, minbytes, timeout)

#define EUART_AREAD(task, reader, minbytes) \
    EUART_AREADT(task, reader, minbytes, 0)


#endif  // EUART_H
