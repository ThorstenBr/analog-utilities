#ifndef __V2TYPES_H
#define __V2TYPES_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;

typedef void (*void_jump)(void);

long longmin(long a, long b) {
    if(a < b) return a;
    return b;
}

long longmax(long a, long b) {
    if(a > b) return a;
    return b;
}

#endif /* __V2TYPES_H */

