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
#define UAIO_ARG1 unsigned int
#define UAIO_ARG2 unsigned long
#include "uaio_generic.c"


int
euart_device_init(struct euart_device *d, uart_config_t *config, int no, \
        int txpin, int rxpin, int flags) {
    // int intr_alloc_flags = 0;
	// #if CONFIG_UART_ISR_IN_IRAM
	//     intr_alloc_flags = ESP_INTR_FLAG_IRAM;
	// #endif
    // esp_vfs_dev_uart_port_set_tx_line_endings(UART_NUM_0,
    //         esp_line_endings_t mode);

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

    return 0;
}


int
euart_reader_init(struct euart_reader *reader, int fd, uint8_t ringmaskbits) {
    if (reader == NULL) {
        return -1;
    }

    reader->fd = fd;

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


/** Read from UART device until the `reader->minbytes`.
 * */
ASYNC
euart_readA(struct uaio_task *self, struct euart_reader *r,
        unsigned int minbytes, unsigned long timeout_us) {
    int ret;
    struct u8ring *ring = &r->ring;
    UAIO_BEGIN(self);

    if (minbytes >= ring->mask) {
        r->status = EUTS_ERROR;
        UAIO_THROW2(self, EINVAL);
    }

    r->status = EUTS_OK;
    while (ERING_USED(ring) < minbytes) {
        errno = 0;

        if (ERING_AVAIL(ring) == 0) {
            r->status = EUTS_ERROR;
            UAIO_THROW2(self, ENOBUFS);
        }

        ret = read(r->fd, ERING_HEADPTR(ring), 1);
        if (ret > 0) {
            ERING_INCRHEAD(ring);
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
        if (timeout_us) {
            /* wait until timeout */
            UAIO_FILE_TWAIT(self, r->fd, UAIO_IN, timeout_us);
            if (UAIO_FILE_TIMEDOUT(self)) {
                r->status = EUTS_TIMEDOUT;
                break;
            }
        }
        else {
            /* wait until data available */
            UAIO_FILE_AWAIT(self, r->fd, UAIO_IN);
        }

        /* file is ready, continue reading... */
    }

    UAIO_FINALLY(self);
    errno = 0;
}
