#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>


#define MUNI_PROJECT_NAME "Muni"

#define LCD_XRES 160
#define LCD_YRES 144

#define SIZE_KIB 1024
#define SIZE_MIB 1024 * SIZE_KIB

#define BIT(a, n) (((a) >> (n)) & 1u)
#define BIT_SET(a, n) ((a) |= (1u << (n)))
#define BIT_CLEAR(a, n) ((a) &= ~(1u << (n)))
#define IN_RANGE(a, base, size) ((uint16_t)(a) >= (uint16_t)(base) && (uint16_t)(a) < (uint16_t)(base) + (size))

#define NO_IMPL { fprintf(stderr, "NOT YET IMPLEMENTED\n"); exit(-5); }

typedef enum {
    SYSTEM_DMG,
    SYSTEM_CGB,
    SYSTEM_SGB
} GbSystemType;

typedef enum {
    SUPPORT_MODE_CGB,
    SUPPORT_MODE_DMG
} GbSupportMode;

#endif
