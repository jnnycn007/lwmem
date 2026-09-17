/**
 * \file            lwmem_test_calloc_simple.c
 * \author          Tilen MAJERLE <tilen@majerle.eu>
 * \brief           
 * \version         0.1
 * \date            2026-09-17
 * 
 * @copyright Copyright (c) 2026
 * 
 * Test code for the size checks of the simple configuration, a LwMEM config
 * that does not support any memory free operation (a lite mode).
 *
 * It covers the "nitems * size" overflow of "lwmem_calloc_ex", and the
 * overflow of the alignment operation applied on the requested size.
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
#if LWMEM_CFG_FULL
#error "Test shall run with LWMEM_CFG_FULL == 0"
#endif

/* Same wrapping operand pairs as used by the full configuration test */
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

/*
 * Alignment bits, the very same value the library derives internally.
 * Any requested size above "SIZE_MAX - TEST_ALIGN_BITS" overflows the
 * alignment operation down to "0"
 */
#define TEST_ALIGN_BITS       ((size_t)LWMEM_CFG_ALIGN_NUM - (size_t)1)
#define TEST_ALIGN_OVF_MIN    (SIZE_MAX - TEST_ALIGN_BITS + (size_t)1)

/* Region memory declaration, use uint32 for alignment reasons */
static uint32_t lw_c_mem1[64 / 4];

/* Regions descriptor */
static const lwmem_region_t lw_c_regions[] = {
    {lw_c_mem1, sizeof(lw_c_mem1)},
    {NULL, 0},
};

int
test_run(void) {
    static lwmem_t lwmem;
    uint8_t* ptr = NULL;
    size_t avail_start = 0;

    /* Verify the test variant really got compiled for the expected width */
    TEST_ASSERT(sizeof(size_t) == TEST_SIZE_T_BYTES);

    /* Verify the test vectors themselves really do wrap around */
    TEST_ASSERT((TEST_OVF_ZERO_NITEMS * TEST_OVF_ZERO_SIZE) == (size_t)0);
    TEST_ASSERT((TEST_OVF_SMALL_NITEMS * TEST_OVF_SMALL_SIZE) == (size_t)16);
    TEST_ASSERT((TEST_ALIGN_OVF_MIN + TEST_ALIGN_BITS) == (size_t)0);

    /* Setup the values */
    TEST_ASSERT(lwmem_assignmem_ex(&lwmem, lw_c_regions) != 0);
    avail_start = lwmem.mem_available_bytes;
    TEST_ASSERT(avail_start > 0);

    /*
     * Alignment of the requested size must not overflow down to "0".
     * Every size in the top "LWMEM_CFG_ALIGN_NUM - 1" range does so, and
     * without the check the allocator returns a pointer while reserving
     * no memory at all
     */
    ptr = lwmem_malloc_ex(&lwmem, NULL, SIZE_MAX);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_malloc_ex(&lwmem, NULL, TEST_ALIGN_OVF_MIN);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /* One below that range does not overflow, but is still far too large */
    ptr = lwmem_malloc_ex(&lwmem, NULL, TEST_ALIGN_OVF_MIN - (size_t)1);
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

    /* Zero-length allocation reserves nothing, it must not return a pointer */
    ptr = lwmem_malloc_ex(&lwmem, NULL, 0);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /* Multiplication overflow of calloc, same as in the full configuration */
    ptr = lwmem_calloc_ex(&lwmem, NULL, TEST_OVF_ZERO_NITEMS, TEST_OVF_ZERO_SIZE);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, TEST_OVF_SMALL_NITEMS, TEST_OVF_SMALL_SIZE);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, TEST_OVF_SMALL_SIZE, TEST_OVF_SMALL_NITEMS);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, 0, 8);
    TEST_ASSERT(ptr == NULL);
    ptr = lwmem_calloc_ex(&lwmem, NULL, 8, 0);
    TEST_ASSERT(ptr == NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == avail_start);

    /* None of the rejected calls may have consumed any memory, all 64 bytes
       must still be up for grabs, and the returned block must be zeroed */
    ptr = lwmem_calloc_ex(&lwmem, NULL, 4, 8);
    TEST_ASSERT(ptr != NULL);
    for (size_t idx = 0; idx < (4 * 8); ++idx) {
        TEST_ASSERT(ptr[idx] == 0x00);
    }
    TEST_ASSERT(lwmem.mem_available_bytes == (avail_start - (size_t)32));

    ptr = lwmem_malloc_ex(&lwmem, NULL, 32);
    TEST_ASSERT(ptr != NULL);
    TEST_ASSERT(lwmem.mem_available_bytes == 0);

    /* Region is used up by now */
    ptr = lwmem_malloc_ex(&lwmem, NULL, 4);
    TEST_ASSERT(ptr == NULL);

    return 0;
}
