#include "PIN_MAP.h"

void pin_function(PinName pin, int function) {
    int index    = pin >> PORT_SHIFT;
    int raw_ofst = pin & 0x00FF;
    int offset;

    //MBED_ASSERT(pin != (PinName)NC);
    int ver = 10;//rda_ccfg_hwver();
    switch(index) {
        case 1:
            if((8 == raw_ofst) && (ver >= 5)) {
                function ^= 0x01;
            }
            break;
        case 4:
            if(1 < raw_ofst) {
                offset = raw_ofst << 1;
                RDA_PINCFG->MODE2 &= ~(0x03UL << offset);
            }
            break;
        case 5:
            if(2 > raw_ofst) {
                offset = (raw_ofst << 1) + 20;
                RDA_PINCFG->MODE2 &= ~(0x03UL << offset);
            } else {
                offset = (raw_ofst << 1) - 4;
                RDA_PINCFG->MODE3 &= ~(0x03UL << offset);
            }
            break;
        default:
            break;
    }

    offset = raw_ofst * 3;
    RDA_PINCFG->IOMUXCTRL[index] &= ~(0x07UL << offset);
    RDA_PINCFG->IOMUXCTRL[index] |= ((function & 0x07UL) << offset);
}

void pin_mode(PinName pin, PinMode mode) 
{
    //MBED_ASSERT(pin != (PinName)NC);
}


void pinmap_pinout(PinName pin, const PinMap *map)
{
    if (pin == NC)
		{
        return;
		}

    while (map->pin != NC) 
		{
        if (map->pin == pin) 
				{
            pin_function(pin, map->function);
            pin_mode(pin, PullNone);
            return;
        }
        map++;
    }
    //error("could not pinout");
}

uint32_t pinmap_merge(uint32_t a, uint32_t b)
{
    // both are the same (inc both NC)
    if (a == b)
		{
        return a;
		}

    // one (or both) is not connected
    if (a == (uint32_t)NC)
		{
        return b;
		}
		
    if (b == (uint32_t)NC)
		{
        return a;
		}

    // mis-match error case
    //error("pinmap mis-match");
    return (uint32_t)NC;
}

uint32_t pinmap_find_peripheral(PinName pin, const PinMap* map)
{
    while (map->pin != NC) 
		{
        if (map->pin == pin)
				{
            return map->peripheral;
				}
        map++;
    }
    return (uint32_t)NC;
}

uint32_t pinmap_peripheral(PinName pin, const PinMap* map)
{
    uint32_t peripheral = (uint32_t)NC;

    if (pin == (PinName)NC)
		{
        return (uint32_t)NC;
		}
		
    peripheral = pinmap_find_peripheral(pin, map);
    if ((uint32_t)NC == peripheral) // no mapping available
		{
			  return (uint32_t)NC;
        //error("pinmap not found for peripheral");
		}
				;
    return peripheral;
}

uint32_t pinmap_find_function(PinName pin, const PinMap* map)
{
    while (map->pin != NC) 
		{
        if (map->pin == pin)
				{
            return map->function;
				}
        map++;
    }
    return (uint32_t)NC;
}

uint32_t pinmap_function(PinName pin, const PinMap* map) 
{
    uint32_t function = (uint32_t)NC;

    if (pin == (PinName)NC)
		{
        return (uint32_t)NC;
		}
    function = pinmap_find_function(pin, map);
    if ((uint32_t)NC == function) // no mapping available
		{
        //error("pinmap not found for function");
				;
		}
    return function;
}
