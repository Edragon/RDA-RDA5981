#include "mbed.h"
#include "rtos.h"
#if defined(TOOLCHAIN_GCC_ARM)
#include <malloc.h>
#endif

void dump_heap_info(void)
{
#if defined(TOOLCHAIN_ARM)
    __heapstats((__heapprt)fprintf,stderr);
    __heapvalid((__heapprt) fprintf, stderr, 1);
#elif defined(TOOLCHAIN_GCC_ARM)
#if 0
// declared in <malloc.h>
struct mallinfo {
    size_t arena;    /* total space allocated from system */
    size_t ordblks;  /* number of non-inuse chunks */
    size_t smblks;   /* unused -- always zero */
    size_t hblks;    /* number of mmapped regions */
    size_t hblkhd;   /* total space in mmapped regions */
    size_t usmblks;  /* unused -- always zero */
    size_t fsmblks;  /* unused -- always zero */
    size_t uordblks; /* total allocated space */
    size_t fordblks; /* total non-inuse space */
    size_t keepcost; /* top-most, releasable (via malloc_trim) space */
};
#endif
    struct mallinfo info = {0,};
    //malloc_stats();
    info = mallinfo();
    printf("total system space   : %d\r\n",info.arena);
    printf("total allocated space: %d\r\n",info.uordblks);
    printf("total non-inuse space: %d\r\n",info.fordblks);
    printf("number of non-inuse chunks   : %d\r\n",info.ordblks);
    printf("biggest non-inuse chunk space: %d\r\n",info.keepcost);
#else
#error "Not Support!"
#endif
}

int main()
{
    void *ptr1;
    void *ptr2;

    printf("\r\nStart heap_info test\r\n");
    dump_heap_info();

    printf("\r\nAllocate 100 bytes\r\n");
    ptr1 = malloc(100);
    dump_heap_info();

    printf("\r\nAllocate 1024 bytes\r\n");
    ptr2 = malloc(1024);
    dump_heap_info();

    printf("\r\nFree 100 bytes\r\n");
    free(ptr1);
    dump_heap_info();

    printf("\r\nFree 1024 bytes\r\n");
    free(ptr2);
    dump_heap_info();

    printf("\r\nAllocate 2048 bytes\r\n");
    ptr1 = malloc(2048);
    dump_heap_info();

    printf("\r\nAllocate 4096 bytes\r\n");
    ptr2 = malloc(4096);
    dump_heap_info();

    printf("\r\nFree 2048 bytes\r\n");
    free(ptr1);
    dump_heap_info();

    printf("\r\nFree 4096 bytes\r\n");
    free(ptr2);
    dump_heap_info();

    Thread::wait(osWaitForever);
}
