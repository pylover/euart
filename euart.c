#include <fcntl.h>
#include <errno.h>

#include <elog.h>
#include <uaio.h>

#include "driver/uart_vfs.h"
#include "driver/uart.h"

#include "euart.h"


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY euart
#define UAIO_ARG1 struct euart_chat *
#include "uaio_generic.c"


int
euart_init(struct euart *u, int no, int txpin, int rxpin, int flags) {
    // int intr_alloc_flags = 0;
	// #if CONFIG_UART_ISR_IN_IRAM
	//     intr_alloc_flags = ESP_INTR_FLAG_IRAM;
	// #endif

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // tx, rx, rts, cts
    if (uart_set_pin(no, txpin, rxpin, UART_PIN_NO_CHANGE,
                UART_PIN_NO_CHANGE)) {
        return -1;
    }

    if (uart_driver_install(no, CONFIG_EUART_BUFFSIZE, 0, 0,
            NULL, 0) != ESP_OK) {
        ERROR("uart driver installation failed");
        return -1;
    }

    ESP_ERROR_CHECK(uart_param_config(no, &uart_config));

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
    return 0;
}


ASYNC
euart_getcA(struct uaio_task *self, struct euart *u, struct euart_chat *c) {
    int ret;
    UAIO_BEGIN(self);

doread:
    errno = 0;
    ret = read(u->infd, &c->result.uint8, 1);
    if (ret == 1) {
        c->status = EUCS_OK;
    }
    else if ((ret == -1) && UAIO_MUSTWAIT(errno)) {
        goto dowait;
    }
    else {
        c->status = EUCS_ERROR;
    }

    UAIO_RETURN(self);

dowait:
    if (c->timeout_us) {
        UAIO_FILE_TWAIT(self, u->infd, UAIO_IN, c->timeout_us);
        if (UAIO_FILE_TIMEDOUT(self)) {
            c->status = EUCS_TIMEDOUT;
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
euart_readA(struct uaio_task *self, struct euart *u, struct euart_chat *c) {
    struct euart_chat *subchat = malloc(sizeof(struct euart_chat));
    memset(subchat, 0, sizeof(struct euart_chat));
    subchat->timeout_us = c->timeout_us;
    subchat->query.count = 1;
    c->userptr = subchat;
    UAIO_BEGIN(self);

    if (c->userptr == NULL) {
        UAIO_THROW(self, ENOMEM);
    }

    while (c->query.count) {
        UAIO_AWAIT(self, euart, euart_getcA, u, c->userptr);
        if (subchat->status != EUCS_OK) {
            c->status = subchat->status;
            break;
        }
        c->query.count--;
    }

    UAIO_FINALLY(self);
    if (subchat) {
        free(subchat);
        c->userptr = NULL;
    }
}


// ASYNC
// euart_retryA(struct uaio_task *self, struct euart *u,
//         struct euart_chat *c) {
//     UAIO_BEGIN(self);
//     c->query.str = "AT";
//     c->flags = MQF_IGNOREEMPTYLINES;
//     c->status = MQS_TOUT;
//     c->timeout_us = 1000000;
//     c->answersize = 0;
//     UAIO_AWAIT(self, euart, euart_dialogueA, u->uart, c);
//     while (c->status != MQS_OK) {
//         UAIO_SLEEP(self, c->);
//         c->answersize = 0;
//         UAIO_AWAIT(self, modem, modem_queryA, m, c);
//     }
//     UAIO_FINALLY(self);
// }
