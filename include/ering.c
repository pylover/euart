#include <stdlib.h>
#include <unistd.h>


#define _MIN(a, b) ((a) < (b)? (a): (b))


int
ERING_NAME1(_init) (struct ERING_NAME() *ring, uint8_t maskbits) {
    unsigned int mask = 1;

    if ((maskbits == 0) || (maskbits > 16)) {
        return -1;
    }
    mask <<= maskbits;
    mask -= 1;

    if (mask == 0) {
        return -1;
    }

    ring->mask = mask;
    ring->buffer = malloc(ERING_BYTES(ring->mask + 1));
    if (ring->buffer == NULL) {
        ERROR("bits: %d mask: %d", maskbits, mask);
        return -1;
    }

    ring->r = 0;
    ring->w = 0;
    return 0;
}


int
ERING_NAME1(_deinit) (struct ERING_NAME() *ring) {
    if (ring == NULL) {
        return -1;
    }

    if (ring->buffer != NULL) {
        free(ring->buffer);
        ring->buffer = NULL;
    }

    return 0;
}


int
ERING_NAME1(_popwrite) (struct ERING_NAME() *ring, int fd, size_t count) {
    int bytes;
    int toend;
    int toread;
    int ret = 0;

    if (count == 0) {
        return 0;
    }

    if (count > ERING_USED(ring)) {
        return -1;
    }

    toend = ERING_USED_TOEND(ring);
    if (toend) {
        toread = _MIN(toend, count);
        bytes = write(fd, ring->buffer + ring->r, ERING_BYTES(toread));
        if (bytes < toread) {
            /* eof, error */
            return -1;
        }

        ring->r = ERING_CALC(ring, ring->r + toread);
        count -= toread;
        ret += toread;
    }

    if (count == 0) {
        return ret;
    }

    bytes = write(fd, ring->buffer, ERING_BYTES(count));
    if (bytes != count ) {
        /* eof, error */
        return -1;
    }

    ring->r = ERING_CALC(ring, ring->r + count);
    count -= toend;
    ret += count;

    return ret;
}


// ssize_t
// ERING_NAME(pop) (ERING_T() *q, ERING_TYPE *data, size_t count) {
//     if (ERING_USED(q) < count) {
//         return -1;
//     }
//
//     size_t toend = ERING_USED_TOEND(q);
//     ssize_t total = ERING_MIN(toend, count);
//
//     memcpy(data, q->buffer + q->r, ERING_BYTES(total));
//     q->r = ERING_READER_CALC(q, total);
//     count -= total;
//
//     if (count) {
//         count = ERING_MIN(ERING_USED_TOEND(q), count);
//         memcpy(data + total, q->buffer, ERING_BYTES(count));
//         q->r += count;
//         total += count;
//     }
//
//     return total;
// }
// enum ering_filestatus
// ERING_NAME(popwrite) (ERING_T() *q, int fd, size_t *count) {
//     int used;
//     int toend;
//
// }


// int
// ERING_NAME(deinit) (ERING_T() *q) {
// }
//
//
// int
// ERING_NAME(reset) (ERING_T() *q) {
//     q->r = 0;
//     q->w = 0;
//     return 0;
// }
//
//
// int
// ERING_NAME(put) (ERING_T() *q, const ERING_TYPE *data, size_t count) {
//     if (ERING_AVAILABLE(q) < count) {
//         return -1;
//     }
//
//     size_t toend = ERING_FREE_TOEND(q);
//     size_t chunklen = ERING_MIN(toend, count);
//     memcpy(q->buffer + q->w, data, ERING_BYTES(chunklen));
//     q->w = ERING_WRITER_CALC(q, chunklen);
//
//     if (count > chunklen) {
//         count -= chunklen;
//         memcpy(q->buffer, data + chunklen, ERING_BYTES(count));
//         q->w += count;
//     }
//
//     return 0;
// }
//
//
//
//
// int
// ERING_NAME(skip) (ERING_T() *q, size_t count) {
//     if (count == 0) {
//         return 0;
//     }
//
//     if (ERING_USED(q) < count) {
//         return -1;
//     }
//
//     q->r = ERING_READER_CALC(q, count);
//     return 0;
// }
//
//
// enum ering_filestatus
// ERING_NAME(readput) (ERING_T() *q, int fd, size_t *count) {
//     int avail;
//     int toend;
//     ssize_t firstbytes;
//     ssize_t secondbytes;
//
//     if (ERING_ISFULL(q)) {
//         /* Buffer is full */
//         return CFS_BUFFERFULL;
//     }
//
//     toend = ERING_FREE_TOEND(q);
//     firstbytes = read(fd, q->buffer + q->w, ERING_BYTES(toend));
//     if (firstbytes == 0) {
//         /* EOF */
//         return CFS_EOF;
//     }
//
//     if (firstbytes < 0) {
//         if (EVMUSTWAIT()) {
//             /* Must wait */
//             return CFS_AGAIN;
//         }
//         /* error */
//         return CFS_ERROR;
//     }
//
//     if (count) {
//         *count = firstbytes / ERING_BYTES(1);
//     }
//     q->w = ERING_WRITER_CALC(q, firstbytes / ERING_BYTES(1));
//     avail = ERING_AVAILABLE(q);
//     if (avail == 0) {
//         /* Buffer is full */
//         return CFS_BUFFERFULL;
//     }
//
//     if (firstbytes / ERING_BYTES(1) < toend) {
//         return CFS_OK;
//     }
//     secondbytes = read(fd, q->buffer + q->w, ERING_BYTES(avail));
//     if (secondbytes == 0) {
//         /* EOF */
//         return CFS_EOF;
//     }
//
//     if (secondbytes < 0) {
//         if (EVMUSTWAIT()) {
//             /* Must wait */
//             return CFS_AGAIN;
//         }
//         /* error */
//         return CFS_ERROR;
//     }
//
//     if (count) {
//         *count = (firstbytes + secondbytes) / ERING_BYTES(1);
//     }
//     q->w = ERING_WRITER_CALC(q, secondbytes / ERING_BYTES(1));
//     return CFS_OK;
// }
//
//
