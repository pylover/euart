#include <fcntl.h>
#include <errno.h>

#include <clog.h>
#include <caio/caio.h>
#include <caio/sleep.h>

#include "driver/uart_vfs.h"
#include "driver/uart.h"

#include "euart.h"


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY euart
#define CAIO_ARG1 struct euart_chat *
#include "caio/generic.c"


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

    if (flags & CUIF_STDIO) {
        u->fd = STDIN_FILENO;
    }
    else {
        char path[16];
        sprintf(path, "/dev/uart/%d", no);
        int fd = open(path, O_RDWR);
        if (fd == -1) {
            ERROR("open(%s)", path);
            return -1;
        }

        u->fd = fd;
    }
    return 0;
}


ASYNC
euart_getc(struct caio_task *self, struct euart *u, struct euart_chat *c) {
    CAIO_BEGIN(self);
    while (true) {
        if (c->timeout_us) {
            CAIO_FILE_TWAIT(self, u->fd, CAIO_IN, c->timeout_us);
            if (CAIO_FILE_TIMEDOUT(self)) {
                c->status = CUCS_TIMEDOUT;
                break;
            }
        }
        else {
            CAIO_FILE_AWAIT(self, u->fd, CAIO_IN);
        }

        if (read(u->fd, &c->result.uint8, 1) == 1) {
            c->status = CUCS_OK;
            break;
        }
    }
    CAIO_FINALLY(self);
}


ASYNC
euart_readA(struct caio_task *self, struct euart *u, struct euart_chat *c) {
    struct euart_chat *subchat = malloc(sizeof(struct euart_chat));
    memset(subchat, 0, sizeof(struct euart_chat));
    subchat->timeout_us = c->timeout_us;
    subchat->query.count = 1;
    c->userptr = subchat;
    CAIO_BEGIN(self);

    if (c->userptr == NULL) {
        CAIO_THROW(self, ENOMEM);
    }

    while (c->query.count) {
        CAIO_AWAIT(self, euart, euart_getc, u, c->userptr);
        if (subchat->status != CUCS_OK) {
            c->status = subchat->status;
            break;
        }
        c->query.count--;
    }

    CAIO_FINALLY(self);
    if (subchat) {
        free(subchat);
        c->userptr = NULL;
    }
}


// ASYNC
// euart_retryA(struct caio_task *self, struct euart *u,
//         struct euart_chat *c) {
//     CAIO_BEGIN(self);
//     c->query.str = "AT";
//     c->flags = MQF_IGNOREEMPTYLINES;
//     c->status = MQS_TOUT;
//     c->timeout_us = 1000000;
//     c->answersize = 0;
//     CAIO_AWAIT(self, euart, euart_dialogueA, u->uart, c);
//     while (c->status != MQS_OK) {
//         CAIO_SLEEP(self, c->);
//         c->answersize = 0;
//         CAIO_AWAIT(self, modem, modem_queryA, m, c);
//     }
//     CAIO_FINALLY(self);
// }