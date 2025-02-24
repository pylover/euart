#include <fcntl.h>
#include <errno.h>

#include <driver/uart_vfs.h>

#include <elog.h>
#include <uaio.h>

#include "euart.h"


#undef ERING_PREFIX
#define ERING_PREFIX u8
#include "ering.c"


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY euart_reader
#include "uaio_generic.c"


int
euart_device_init(struct euart_device *d, uart_config_t *config, int no, \
        int txpin, int rxpin, int flags) {
    // int intr_alloc_flags = 0;
	// #if CONFIG_UART_ISR_IN_IRAM
	//     intr_alloc_flags = ESP_INTR_FLAG_IRAM;
	// #endif

    // tx, rx, rts, cts
    if (uart_set_pin(no, txpin, rxpin, UART_PIN_NO_CHANGE,
                UART_PIN_NO_CHANGE) != ESP_OK) {
        return -1;
    }

    if (uart_driver_install(no, CONFIG_EUART_BUFFSIZE, 0, 0,
            NULL, 0) != ESP_OK) {
        return -1;
    }

    if (uart_param_config(no, config) != ESP_OK) {
        return -1;
    }

    uart_vfs_dev_use_driver(no);
    // if (flags & EUIF_NONBLOCK) {
    //     INFO("Using vfs nonblock driver");
    //     uart_vfs_dev_use_nonblocking(no);
    // }

    if (flags & EUIF_STDIO) {
        d->infd = STDIN_FILENO;
        d->outfd = STDOUT_FILENO;
    }
    else {
        char path[16];
        int f = O_RDWR;
        if (flags & EUIF_NONBLOCK) {
            f |= O_NONBLOCK;
        }
        sprintf(path, "/dev/uart/%d", no);
        int fd = open(path, f);
        if (fd == -1) {
            ERROR("open(%s)", path);
            return -1;
        }

        d->infd = fd;
        d->outfd = fd;
    }

    d->no = no;
    d->flags = flags;
    return 0;
}


int
euart_reader_init(struct euart_reader *reader, struct euart_device *dev,
        unsigned int timeout_us, uint8_t ringmaskbits) {
    if (reader == NULL) {
        return -1;
    }

    reader->timeout_us = timeout_us;
    reader->device = dev;

    if (u8ring_init(&reader->ring, ringmaskbits)) {
        return -1;
    }

    return 0;
}


int
euart_reader_deinit(struct euart_reader *reader) {
    if (reader == NULL) {
        return -1;
    }

    return u8ring_deinit(&reader->ring);
}


ASYNC
euart_readA(struct uaio_task *self, struct euart_reader *r) {
    int ret;
    struct euart_device *d = r->device;
    struct u8ring *ring = &r->ring;
    UAIO_BEGIN(self);

    r->status = EUTS_OK;
    while (ERING_AVAIL(ring)) {
        errno = 0;
        ret = read(d->infd, ERING_CUR(ring), 1);
        if (ret > 0) {
            ERING_INCR(ring);
            continue;
        }

        /* eof */
        if (ret == 0) {
            r->status = EUTS_EOF;
            UAIO_THROW(self);
        }

        /* error */
        if ((ret == -1) && (!UAIO_MUSTWAIT(errno))) {
            ERROR("Read error");
            r->status = EUTS_ERROR;
            UAIO_THROW(self);
        }

        /* read again later (nonblocking wait) */
        if (ERING_USED(ring) < r->minbytes) {
            /* wait until data available */
            UAIO_FILE_AWAIT(self, d->infd, UAIO_IN);
        }
        else if (r->timeout_us) {
            /* wait until timeout */
            UAIO_FILE_TWAIT(self, d->infd, UAIO_IN, r->timeout_us);
            if (UAIO_FILE_TIMEDOUT(self)) {
                r->status = EUTS_TIMEDOUT;
                break;
            }
        }
        else {
            /* return immediately */
            break;
        }

        /* the file is ready, continue reading... */
    }

    UAIO_FINALLY(self);
    errno = 0;
}
