#include <fcntl.h>
#include <errno.h>

#include <driver/uart_vfs.h>

#include <elog.h>
#include <uaio.h>

#include "euart.h"


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


ASYNC
euart_readA(struct uaio_task *self, struct euart_reader *r) {
    int ret;
    struct euart_device *d = r->device;
    UAIO_BEGIN(self);

    DEBUG("reading...");
    r->status = EUTS_OK;
    while (r->bytes < r->max) {
        errno = 0;
        ret = read(d->infd, r->buff + r->bytes, 1);
        DEBUG("ret: %d", ret);

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
        /* read again later, nonblocking wait */
        else {
            DEBUG("EAGAIN");
            /* wait until specific timeout */
            if (r->timeout_us) {
                DEBUG("TWAIT: %ld", r->timeout_us);
                UAIO_FILE_TWAIT(self, d->infd, UAIO_IN, r->timeout_us);
                if (UAIO_FILE_TIMEDOUT(self)) {
                    r->status = EUTS_TIMEDOUT;
                    break;
                }
            }
            /* wait forever */
            else {
                DEBUG("WAIT for ever");
                UAIO_FILE_AWAIT(self, d->infd, UAIO_IN);
            }

            /* file is ready, continue reading... */
            continue;
        }

        r->bytes += ret;
    }

    UAIO_FINALLY(self);
    errno = 0;
}
