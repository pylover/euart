#ifndef ERING_COMMON_H_
#define ERING_COMMON_H_


#define ERING_TYPENAME ring


/* Generic stuff */
#define ERING_PASTER2(x, y) x ## y
#define ERING_PASTER3(x, y, z) x ## y ## z
#define ERING_EVALUATOR2(x, y)  ERING_PASTER2(x, y)
#define ERING_EVALUATOR3(x, y, z)  ERING_PASTER3(x, y, z)
#define ERING_NAME() ERING_EVALUATOR2(ERING_PREFIX, ERING_TYPENAME)
#define ERING_NAME1(n) ERING_EVALUATOR3(ERING_PREFIX, ERING_TYPENAME, n)
#define ERING_BUFFTYPE() ERING_EVALUATOR2(ERING_PREFIX, _t)


typedef struct ERING_NAME() {
    unsigned int mask;

    /* Read/Tail*/
    int r;

    /* Write/Head */
    int w;

    ERING_BUFFTYPE() *buffer;
} ERING_NAME1(_t);


#define ERING_BYTES(n) ((n) * sizeof(ERING_BUFFTYPE()))
#define ERING_CALC(b, n) ((n) & (b)->mask)
#define ERING_USED(b) ERING_CALC(b, (b)->w - (b)->r)
#define ERING_AVAIL(b) ERING_CALC(b, (b)->r - ((b)->w + 1))
#define ERING_INCR(b) (b)->w = ERING_CALC(b, (b)->w + 1)
#define ERING_CUR(b) ((b)->buffer + (b)->w)
#define ERING_ISEMPTY(b) (ERING_USED(b) == 0)
#define ERING_ISFULL(b) (ERING_AVAIL(b) == 0)
#define ERING_USED_TOEND(b) \
    ({int end = ((b)->mask + 1) - (b)->r; \
      int n = ((b)->w + end) & (b)->mask; \
      n < end ? n : end;})
#define ERING_SKIP(b, n) (b)->r = ERING_CALC((b), (b)->r + (n))


int
ERING_NAME1(_init) (struct ERING_NAME() *ring, uint8_t maskbits);


int
ERING_NAME1(_deinit) (struct ERING_NAME() *ring);


int
ERING_NAME1(_writeout) (struct ERING_NAME() *ring, int fd, size_t count);


#endif
