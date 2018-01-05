/* mbed Microcontroller Library - stackheap
 * Copyright (C) 2009-2011 ARM Limited. All rights reserved.
 * 
 * Setup a fixed single stack/heap memory model, 
 *  between the top of the RW/ZI region and the stackpointer
 */

#ifdef __cplusplus
extern "C" {
#endif 

#include <rt_misc.h>
#include <stdint.h>

extern char Image$$ARM_LIB_HEAP$$ZI$$Base[];
extern char Image$$ARM_LIB_HEAP$$ZI$$Length[];
extern char Image$$ARM_LIB_STACK$$ZI$$Base[];
extern char Image$$ARM_LIB_STACK$$ZI$$Length[];

extern __value_in_regs struct __initial_stackheap __user_setup_stackheap(uint32_t R0, uint32_t R1, uint32_t R2, uint32_t R3) 
{
    uint32_t hp_base  = (uint32_t)Image$$ARM_LIB_HEAP$$ZI$$Base;
    uint32_t hp_limit = (uint32_t)Image$$ARM_LIB_HEAP$$ZI$$Length + hp_base;
    uint32_t sp_base  = (uint32_t)Image$$ARM_LIB_STACK$$ZI$$Base;
    uint32_t sp_limit = (uint32_t)Image$$ARM_LIB_STACK$$ZI$$Length + sp_base;

    struct __initial_stackheap r;
    r.heap_base = hp_base;
    r.heap_limit = hp_limit;
    r.stack_base = sp_base;
    r.stack_limit = sp_limit;
    return r;
}

#ifdef __cplusplus
}
#endif 
