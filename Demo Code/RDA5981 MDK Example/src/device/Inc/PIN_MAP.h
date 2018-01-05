#ifndef __PIN_MAP_H
#define __PIN_MAP_H

#include "RDA5991H.h"
#include "GPIO.h"

#ifdef __cplusplus
extern "C" {
#endif

void			pin_function(PinName pin, int function);
void			pin_mode    (PinName pin, PinMode mode);

uint32_t	pinmap_peripheral(PinName pin, const PinMap* map);
uint32_t	pinmap_function(PinName pin, const PinMap* map);
uint32_t	pinmap_merge     (uint32_t a, uint32_t b);
void			pinmap_pinout    (PinName pin, const PinMap *map);
uint32_t	pinmap_find_peripheral(PinName pin, const PinMap* map);
uint32_t	pinmap_find_function(PinName pin, const PinMap * map);

#ifdef __cplusplus
}
#endif

#endif
