/**
 * \file            lwmem_test_calloc.c
 * \author          Tilen MAJERLE <tilen@majerle.eu>
 * \brief           
 * \version         0.1
 * \date            2026-09-17
 * 
 * @copyright Copyright (c) 2026
 * 
 * Test code for the "lwmem_calloc_ex" function, with the main focus
 * on the "nitems * size" multiplication overflowing the size_t type.
 *
 * This test assumes 32-bit architecture (4-bytes size_t)
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lwmem/lwmem.h"
#include "test.h"

/* Size of the size_t type this particular test variant is compiled for */
#define TEST_SIZE_T_BYTES 4

#if LWMEM_CFG_ALIGN_NUM != 4
#error "Test shall run with LWMEM_CFG_ALIGN_NUM == 4"
#endif
#if !LWMEM_CFG_FULL
#error "Test shall run with LWMEM_CFG_FULL == 1"
#endif
#if LWMEM_CFG_CLEAN_MEMORY
#error "Test shall run with LWMEM_CFG_CLEAN_MEMORY == 0"
#endif

/*
 * Operand pairs that overflow the "nitems * size" multiplication.
 *
 * Both are derived from SIZE_MAX, therefore the very same expressions
 * hold for a 32-bit and for a 64-bit size_t, with "N" being the number
 * of bits of the type:
 *
 *  - Wrap down to zero:
 *      (SIZE_MAX / 2 + 1) * 2 = 2^N = 0 (mod 2^N)
 *
 *  - Wrap down to a small, non-zero value:
 *      (SIZE_MAX / 16 + 2) * 16 = 2^N + 16 = 16 (mod 2^N)
 *
 *    This is the dangerous one. Without the overflow check, the allocator
 *    hands out a 16-byte block for a request of nearly the whole address
 *    space, and reports success while doing so
 */
/*
 * Top-most bit of the size_t, the very bit the library reserves to mark
 * a block as allocated, and the largest request that stays clear of it
 * once the alignment has been applied on top of the requested size
 */
#define TEST_ALLOC_BIT        ((size_t)1 << (sizeof(size_t) * CHAR_BIT - 1))
#define TEST_MAX_ALLOC_SIZE   (TEST_ALLOC_BIT - (size_t)LWMEM_CFG_ALIGN_NUM)

#define TEST_OVF_ZERO_NITEMS  ((size_t)2)
#define TEST_OVF_ZERO_SIZE    ((SIZE_MAX / (size_t)2) + (size_t)1)
#define TEST_OVF_SMALL_NITEMS ((SIZE_MAX / (size_t)16) + (size_t)2)
#define TEST_OVF_SMALL_SIZE   ((size_t)16)

/* Largest pair that does not overflow, but can never fit in any region */
#define TEST_MAX_NITEMS       (SIZE_MAX / (size_t)16)
#define TEST_MAX_SIZE         ((size_t)16)

int
test_run(void) {
    static lwmem_t lwmem;
    uint32_t data_buff[256 >> 2];
    uint8_t* ptr = NULL;
    size_t avail_start = 0;
    const lwmem_region_t regions[] = {
        {data_buff, sizeof(data_buff)},
        {NULL, 0},
    };

    /* Verify the test variant really got compiled for the expected width */
    TEST_ASSERT(sizeof(size_t) == TEST_SIZE_T_BYTES);

    /*
     * Verify the test vectors themselves, before they are of any use.
     * These must wrap around, otherwise the test below proves nothing
     */
    TEST_ASSERT((TEST_OVF_ZERO_NITEMS * TEST_OVF_ZERO_SIZE) == (size_t)0);
    TEST_ASSERT((TEST_OVF_SMALL_NITEMS * TEST_OVF_SMALL_SIZE) == (size_t)16);
    TEST_ASSERT(TEST_OVF_ZERO_NITEMS > (SIZE_MAX / TEST_OVF_ZERO_SIZE));
    TEST_ASSERT(TEST_OVF_SMALL_NITEMS > (SIZE_MAX / TEST_OVF_SMALL_SIZE));
    TEST_ASSERT(TEST_MAX_NITEMS == (SIZE_MAX / TEST_MAX_SIZE));

    /*
     * Fill the region with a non-zero pattern upfront.
     * A successful calloc has to actively clear the memory it returns
     */
    memset(data_buff, 0xFF, sizeof(data_buff));

    /* Setup the values */
    lwmem_assignmem_ex(&lwmem, regions);
    avail_start = lwmem.mem_available_bytes;
    TEST_ASSERT(avail_start > 0);

    /* Regular allocation must succeed and must come back zeroed */
    ptr = lwmem_calloc_ex(&lwmem, NULL, 4, 8);
    TEST_ASSERT(ptr != NULL);
    TEST_ASSERT(lwmem.mem_available_bytes < avail_start);
    for (size_t idx = 0; idx < (4 * 8); ++idx) {
        TEST_ASSERT(ptr[idx] == 0x00);
    }

    /* Release it, accounting must go back to the starting point */
    lwmem_free_s_ex(&lwmem, (void**)&ptr);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /* Multiplication wraps down to exactly zero, in both operand orders */
    ptr = lwmem_calloc_ex(&lwmem, NULL, TEST_OVF_ZERO_NITEMS, TEST_OVF_ZERO_SIZE);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, TEST_OVF_ZERO_SIZE, TEST_OVF_ZERO_NITEMS);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /* Multiplication wraps down to a small, allocatable value */
    ptr = lwmem_calloc_ex(&lwmem, NULL, TEST_OVF_SMALL_NITEMS, TEST_OVF_SMALL_SIZE);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, TEST_OVF_SMALL_SIZE, TEST_OVF_SMALL_NITEMS);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /* No overflow at all, but far larger than the region */
    ptr = lwmem_calloc_ex(&lwmem, NULL, TEST_MAX_NITEMS, TEST_MAX_SIZE);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /*
     * Requests that the alignment rounds up to, or over, the reserved bit.
     * None of them can ever be served, and none may reach the allocator
     * with the bit already set in the size
     */
    ptr = lwmem_malloc_ex(&lwmem, NULL, TEST_ALLOC_BIT - (size_t)1);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_malloc_ex(&lwmem, NULL, TEST_ALLOC_BIT);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_malloc_ex(&lwmem, NULL, TEST_MAX_ALLOC_SIZE + (size_t)1);
    TEST_ASSERT(ptr == NULL);

    /* Largest accepted size, it simply does not fit in the region */
    ptr = lwmem_malloc_ex(&lwmem, NULL, TEST_MAX_ALLOC_SIZE);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, 1, TEST_MAX_ALLOC_SIZE);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /* Zero on either input has nothing to allocate */
    ptr = lwmem_calloc_ex(&lwmem, NULL, 0, 8);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, 8, 0);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, 0, 0);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /*
     * The overflow check must not reject what used to work.
     * The allocator is still fully usable after all the rejected calls
     */
    ptr = lwmem_calloc_ex(&lwmem, NULL, 1, 1);
    TEST_ASSERT(ptr != NULL);
    TEST_ASSERT(ptr[0] == 0x00);
    lwmem_free_s_ex(&lwmem, (void**)&ptr);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    ptr = lwmem_calloc_ex(&lwmem, NULL, 16, 2);
    TEST_ASSERT(ptr != NULL);
    for (size_t idx = 0; idx < (16 * 2); ++idx) {
        TEST_ASSERT(ptr[idx] == 0x00);
    }
    lwmem_free_s_ex(&lwmem, (void**)&ptr);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    return 0;
}
