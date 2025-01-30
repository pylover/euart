#include <fcntl.h>
#include <errno.h>

#include <driver/uart_vfs.h>

#include <elog.h>
#include <uaio.h>

#include "euart.h"


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY euart
#define UAIO_ARG1 struct euart_query *
#include "uaio_generic.c"


int
euart_init(struct euart *u, uart_config_t *config, int no, int txpin,
        int rxpin, int flags) {
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
        u->infd = STDIN_FILENO;
        u->outfd = STDOUT_FILENO;
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

        u->infd = fd;
        u->outfd = fd;
    }

    u->no = no;
    u->flags = flags;
    return 0;
}


ASYNC
euart_getcA(struct uaio_task *self, struct euart *u, struct euart_getc *g) {
    int ret;
    UAIO_BEGIN(self);

doread:
    errno = 0;
    ret = read(u->infd, &g->c, 1);
    if (ret == 1) {
        g->status = EUCS_OK;
    }
    else if ((ret == -1) && UAIO_MUSTWAIT(errno)) {
        goto dowait;
    }
    else {
        g->status = EUCS_ERROR;
    }

    UAIO_RETURN(self);

dowait:
    if (g->timeout_us) {
        UAIO_FILE_TWAIT(self, u->infd, UAIO_IN, g->timeout_us);
        if (UAIO_FILE_TIMEDOUT(self)) {
            g->status = EUCS_TIMEDOUT;
            UAIO_RETURN(self);
        }
    }
    else {
        UAIO_FILE_AWAIT(self, u->infd, UAIO_IN);
    }

    goto doread;

    UAIO_FINALLY(self);
    errno = 0;
}


ASYNC
euart_readA(struct uaio_task *self, struct euart *u, struct euart_read *r) {
    UAIO_BEGIN(self);

    while (r->bufflen < r->max) {
        EUART_AWAIT(self, euart_getcA, u, r);
        if (r->status != EUCS_OK) {
            break;
        }

        r->buff[r->bufflen++] = r->c;
    }

    r->buff[r->bufflen] = '\0';
    UAIO_FINALLY(self);
}


// ASYNC
// euart_retryA(struct uaio_task *self, struct euart *u,
//         struct euart_chat *c) {
//     UAIO_BEGIN(self);
//     c->query.str = "AT";
//     c->flags = MQF_IGNOREEMPTYLINES;
//     c->status = MQS_TOUT;
//     c->timebufflen = 1000000;
//     c->answersize = 0;
//     UAIO_AWAIT(self, euart, euart_dialogueA, u->uart, c);
//     while (c->status != MQS_OK) {
//         UAIO_SLEEP(self, c->);
//         c->answersize = 0;
//         UAIO_AWAIT(self, modem, modem_queryA, m, c);
//     }
//     UAIO_FINALLY(self);
// }
