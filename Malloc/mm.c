/*
This implementation of malloc uses an explicit free list.

Each block in the heap consists of 3 components: the header, payload, and footer.
The header and footer both contain the size of the entire block, including all
3 components. Since these sizes are multiples of 16, then the last 4 bits of
this variable are guarenteed to be 0, so the last bit is used to store whether
the block is allocated or not. The header and footer are used for forward and
reverse traversal of the heap.

There are 4 global variables used in the program. Heap_first stores the pointer
of the first block in the heap, whle heap_last stores the pointer to the last
block in the heap. free_block_first stores the pointer to the first block in the
explicit free list, while free_block_last stores the pointer to the last block
in the explicit free list. These 4 variables use 32 bytes of memory total.

As blocks are freed, the program attempts to coalesce the freed block with
neighboring blocks which are also unallocated. These free blocks are added to
the beginning of the free list.When looking for appropriately sized blocks that
the program could reuse for calls to malloc, the program searches the list from
the front first. This means the implementation uses a first in first out
approach. When an appropriate block is found, the program attempts to split it
into two smaller blocks so the remaining portion of memory can be reused.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
//#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif



/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

#define ALIGNMENT (2 * sizeof(size_t))

typedef struct {
    size_t header;
    uint8_t payload[];
} block_t;

typedef struct{
    size_t footer;
} footer_t;

typedef struct{
    block_t *prev;
    block_t *next;
} free_list_t;

static block_t *heap_first = NULL;
static block_t *heap_last = NULL;
static block_t *free_block_first = NULL;
static block_t *free_block_last = NULL;


// Returns true if ptr is between mem_heap_lo and mem_heap_hi, false otherwise
static bool ptr_in_range(void *ptr){
    return ptr != NULL && (char*)ptr - (char*)mem_heap_lo >= 0 && (char*)mem_heap_hi() - (char*)ptr >= 0;
}

// Returns the smallest integer above size which is a multiple of n
static size_t round_up(size_t size, size_t n) {
    return (size + n - 1) / n * n;
}

// Returns the size of a block given a block pointer
// This size includes the size of the header, payload, and footer
static size_t get_size(block_t *block) {
    return block->header & ~0xF;
}

// Returns true if the given block is allocated, false otherwise
static bool get_allocated(block_t *block){
    return block->header & 0x1;
}

// Returns the size of the previous block
// Assumes it is safe to check the size of the previous block.
static size_t get_prev_size(block_t *block){
    footer_t *ptr = (footer_t*)((char*)block - sizeof(footer_t));
    return ptr->footer & ~0xF;
}


// Returns a pointer to the next block in the heap
// If the block being checked is the last block, returns NULL instead
static block_t *next_block(block_t *block){
    if(block != heap_last){
        size_t size = get_size(block);
        return (block_t*)((char*)block + size);
    } else {
        return NULL;
    }
}

// Returns a pointer to the previous block in the heap
// If the block being checked is the first block, returns NULL instead
static block_t *prev_block(block_t *block){
    if(block != heap_first){
        size_t size = get_prev_size(block);
        return (block_t*)((char*)block - size);
    } else {
        return NULL;
    }
}

// Calculates the pointer to the footer of a block given the size of the block
// Size includes the size of the header, payload, and footer
footer_t *get_footer(block_t *block, size_t block_size){
    return (footer_t*)((char*)block + block_size - sizeof(footer_t));
}

// Sets the header and footer of a block to contain the given size
// Since the size is a multiple of ALIGNMENT (16), then the last 4 bits will
// always be 0. The last bit is used as an indicator to see if the block is
// allocated or not.
static void set_header_and_footer(block_t *block, size_t block_size, bool is_allocated) {
    dbg_printf("Set Header and footer: %p\tSize: %zu\tAllocated: %d\n", block, block_size, is_allocated);
    block->header = block_size | is_allocated;
    footer_t *footer = get_footer(block, block_size);
    footer->footer = block_size | is_allocated;
}

// Returns a pointer to the free list of a free block
static free_list_t *free_list_ptr(block_t *free_block){
    return (free_list_t*)free_block->payload;
}

// Assumes the input is a valid free block
// Returns the next free block in the explicit free list
// Returns NULL if the input is the last free block in the list
static block_t *next_free_block(block_t *free_block){
    free_list_t *ptr = free_list_ptr(free_block);
    return ptr->next;
}

// Assumes the input is a valid free blocks
// Returns the previosu free block in the explicit free list
// Returns NULL if the input is the first free block in the list
static block_t *prev_free_block(block_t *free_block){
    free_list_t *ptr = free_list_ptr(free_block);
    return ptr->prev;
}

// Returns the size of the payload necessary to be a valid payload, given then
// requested payload size
static size_t get_payload_size(size_t size){
    /*We must have at least sizeof(free_list_t) in the payload so that we can store
    store the pointers to the previous and next free blocks in the payload if
    the block is freed
    */
    if (size < sizeof(free_list_t)){
        return sizeof(free_list_t);
    } else {
        return size;
    }
}

// Returns the minimum size of a block necessary for a payload of size, including
// the header, payload and footer
static size_t get_block_size(size_t size){
    return round_up(sizeof(block_t) + get_payload_size(size) + sizeof(footer_t), ALIGNMENT);
}

// Connects two non NULL free blocks to each other
static void connect_free_blocks(block_t *free_block_1, block_t *free_block_2){
    dbg_printf("Connecting blocks: %p and %p\n", free_block_1, free_block_2);
    free_list_t *free_list_1 = free_list_ptr(free_block_1);
    free_list_t *free_list_2 = free_list_ptr(free_block_2);
    free_list_1->next = free_block_2;
    free_list_2->prev = free_block_1;
}

// Assumes free_block is inside the explicit free list
// Removes free_block from the free list while appropriately updating
// free_block_first, free_block_last, and the previous and next pointers of the
// surrounding pointers
static void remove_from_free_list(block_t *free_block){
    free_list_t *free_list = free_list_ptr(free_block);
    if(free_block == free_block_first || free_block == free_block_last){
        if(free_block_first == free_block_last){
            free_block_first = NULL;
            free_block_last = NULL;
        } else if(free_block == free_block_first){
            free_list_t *next_free_list = free_list_ptr(free_list->next);
            next_free_list->prev = NULL;
            free_block_first = free_list->next;
        } else if(free_block == free_block_last){
            free_list_t *prev_free_list = free_list_ptr(free_list->prev);
            prev_free_list->next = NULL;
            free_block_last = free_list->prev;
        }
    } else {
        connect_free_blocks(free_list->prev, free_list->next);
    }
    dbg_printf("After removing %p, free first is %p\t free last is %p\n", free_block, free_block_first, free_block_last);
}

// Adds the given free block to the beginning of the free list
// Appropriately updates free_block_first and free_block_last and the prev/next
// pointers
static void add_to_start_free_list(block_t *free_block){
    if(free_block_first != NULL){
        free_list_t *free_list = free_list_ptr(free_block);
        free_list_t *first_list = free_list_ptr(free_block_first);
        free_list->next = free_block_first;
        free_list->prev = NULL;
        first_list->prev = free_block;
    }
    free_block_first = free_block;
    if(!free_block_last){
        free_block_last = free_block;
    }
}

// free_block_1 and free_block_2 are assumed to both be free blocks
// Makes free_block_1 a larger block which includes the size of free_block_2
static block_t *coalesce_adjacent_blocks(block_t *free_block_1, block_t *free_block_2){
    dbg_printf("Going to coalesced block %p with %p\n", free_block_1, free_block_2);
    remove_from_free_list(free_block_2);
    set_header_and_footer(free_block_1, get_size(free_block_1) + get_size(free_block_2), false);

    if (heap_last == free_block_2){
        heap_last = free_block_1;
    }
    dbg_printf("Coalesce complete\n");
    return free_block_1;
}

// Tries to coalesce free_block with the surrounding prev and next blocks
static void coalesce_free_block(block_t* free_block){
    dbg_printf("Trying to coalesce block %p ...\n", free_block);
    block_t *prev = prev_block(free_block);
    block_t *next = next_block(free_block);
    dbg_printf("Prev block: %p\n", prev);
    dbg_printf("Next block: %p\n", next);
    block_t *curr_block = free_block;
    if (next != NULL && !get_allocated(next)){
        curr_block = coalesce_adjacent_blocks(free_block, next);
    }
    if (prev != NULL && !get_allocated(prev)){
        curr_block = coalesce_adjacent_blocks(prev, free_block);
    }
}

// Tries to split free_block into a block of new_block_size if possible
// It is only possible if the remaining block would have enough space to be
// a valid block which can contain some amount of bytes
// If a split does occur, free_block now has size new_block_size whose next
// block is a free block with the remaining size
static void split(block_t *free_block, size_t new_block_size){
    dbg_printf("Trying to split block: %p...\n", free_block);
    size_t old_block_size = get_size(free_block);
    remove_from_free_list(free_block);
    if (old_block_size - new_block_size >= get_block_size(1)){
        dbg_printf("Can split, Old size: %zu\tNew size: %zu\n", old_block_size, new_block_size);
        size_t remaining_block_size = old_block_size - new_block_size;
        set_header_and_footer(free_block, new_block_size, false);
        block_t *new_block = (block_t*)((char*)free_block + new_block_size);
        dbg_printf("Remaining block address: %p\n", new_block);
        set_header_and_footer(new_block, remaining_block_size, false);

        free_list_t *new_block_free_list = free_list_ptr(new_block);
        new_block_free_list->prev = NULL;
        new_block_free_list->next = NULL;

        add_to_start_free_list(new_block);
        if(heap_last == free_block){
            heap_last = new_block;
        }
    }
}

// Returns a free block of the appropriate size (block_size) if it exists in
// the free list. Automatically splits the block if it can.
// Returns NULL if a block cannot be found.
static block_t *find_fit(size_t block_size){
    dbg_printf("Trying to find fit for %zu...\n", block_size);
    for(block_t *curr_block = free_block_first; curr_block != NULL; curr_block = next_free_block(curr_block)){
        if (get_size(curr_block) >= block_size){
            dbg_printf("Found fit!\n");
            split(curr_block, block_size);
            return curr_block;
        }
    }
    dbg_printf("Didn't find fit...\n");
    return NULL;
}

// Called whenever a new trace is run, sets up the necessary variables
int mm_init(void) {
    dbg_printf("Initializing...\n");
    /* Pad heap start so first payload is at ALIGNMENT. */
    if ((long) mem_sbrk(ALIGNMENT - offsetof(block_t, payload)) < 0) {
        return -1;
    }
    heap_first = NULL;
    heap_last = NULL;
    free_block_first = NULL;
    free_block_last = NULL;

    return 0;
}

// Returns a pointer which is guarenteed to contain size bytes of usable space
void *malloc(size_t size) {
    dbg_printf("Starting to malloc...\n");
    size_t block_size = get_block_size(size);
    block_t *block = find_fit(block_size);
    if (block == NULL){
        block = mem_sbrk(block_size);
        if ((long) block < 0) {
            return NULL;
        }
        heap_last = block;
        set_header_and_footer(block, block_size, true);
    } else {
        set_header_and_footer(block, get_size(block), true);
    }
    if(!heap_first){
        heap_first = block;
    }

    return block->payload;
}

// Marks a pointer as no longer in use, and can be reclaimed by the program
void free(void *ptr) {
    if(ptr_in_range(ptr)){
        block_t *block = (block_t*)((char*)ptr - sizeof(block_t));
        dbg_printf("Starting to free %p...\n", block);
        set_header_and_footer(block, get_size(block), false);

        free_list_t *new_last_free_list = free_list_ptr(block);
        new_last_free_list->next = NULL;
        new_last_free_list->prev = NULL;

        add_to_start_free_list(block);
        coalesce_free_block(block);
    }
}

// Copies the data from old_ptr to a new block which has size size.
void *realloc(void *old_ptr, size_t size) {
    dbg_printf("Realloccing...\n");
    if (size == 0) {
        free(old_ptr);
        return NULL;
    }

    if (!old_ptr) {
        return malloc(size);
    }

    block_t *block = old_ptr - offsetof(block_t, payload);
    size_t old_size = get_size(block);
    if(old_size >= get_block_size(size)){
        return old_ptr;
    }

    void *new_ptr = malloc(size);

    if (!new_ptr) {
        return NULL;
    }


    if (size < old_size) {
        old_size = size;
    }
    memcpy(new_ptr, old_ptr, old_size);

    free(old_ptr);

    return new_ptr;
}


// Allocate a block while setting the values of the bytes to 0.
void *calloc(size_t nmemb, size_t size) {
    dbg_printf("Callocing...\n");
    size_t bytes = nmemb * size;
    void *new_ptr = malloc(bytes);

    /* If malloc() fails, skip zeroing out the memory. */
    if (new_ptr) {
        memset(new_ptr, 0, bytes);
    }

    return new_ptr;
}

// Checks the following:
// Pointer addresses are appropriately aligned.
// Header and footer of a block are equal.
// All free blocks are in the explicit list.
// There are no 2 adjacent free blocks in the heap.
// Forward and reverse traversal of the heap is consistent.
// All blocks in the free list are in the heap.
// The free list contains no allocated blocks.
// Previous and next pointers are consistent in the free list.
// The number of free blocks in the heap and the explicit list are equal.
void mm_checkheap(int lineno) {
    (void) lineno;
    int num_blocks_forward = 0;
    int num_blocks_backward = 0;
    int num_free_blocks = 0;
    bool prev_block_allocated = true;
    dbg_printf("Starting block search...\n");
    dbg_printf("Heap First pointer: %p\n", heap_first);
    dbg_printf("Heap Last pointer: %p\n", heap_last);
    for(block_t *current_block = heap_first; current_block != NULL; current_block = next_block(current_block)){
        dbg_printf("Current pointer: %p\tSize:%zu\tAllocated: %d\n", current_block, get_size(current_block), get_allocated(current_block));
        void *ptr = (void*)current_block->payload;
        num_blocks_forward++;

        //Check pointer address alignment
        if(((size_t)ptr & 0xF) != 0){
            printf("ERROR: Pointer address not aligned at line %d\n", lineno);
            exit(1);
        }

        //Check header and footer are equal (both size and allocated bit)
        footer_t *footer = get_footer(current_block, get_size(current_block));
        if (footer->footer  != current_block->header){
            printf("ERROR: Header and Footer are not equal at line %d\n", lineno);
            exit(1);
        }
        if(!get_allocated(current_block)){
            //Check if an unallocated block is in the free list
            bool is_in_list = false;
            for(block_t *curr_free_block = free_block_first; curr_free_block != NULL; curr_free_block = next_free_block(curr_free_block)){
                is_in_list = is_in_list || curr_free_block == current_block;
            }
            if(!is_in_list){
                printf("ERROR: Found a free block that is not in the list at line %d\n", lineno);
                exit(1);
            }

            //Make sure there are no two consecutive free blocks
            if(!prev_block_allocated){
                printf("ERROR: Found two adjacent free blocks at line %d\n", lineno);
                exit(1);
            }
            num_free_blocks++;
        }
        prev_block_allocated = get_allocated(current_block);
    }
    dbg_printf("Finished first block search...\n");
    for(block_t *current_block = heap_last; current_block != NULL; current_block = prev_block(current_block)){
        dbg_printf("Current pointer: %p\tSize:%zu\n", current_block, get_size(current_block));
        num_blocks_backward++;
    }

    //Check to see if traversal of heap forwards and backwards is consistent
    if(num_blocks_forward != num_blocks_backward){
        printf("ERROR: Number of blocks in both directions is inconsistent at line %d\n", lineno);
        exit(1);
    }
    // Checking free list
    int num_free_blocks_list = 0;
    block_t *previous_free_block = NULL;
    for(block_t *curr_free_block = free_block_first; curr_free_block != NULL; curr_free_block = next_free_block(curr_free_block)){
        //Check that this is a valid pointer in the heap
        if(!ptr_in_range(curr_free_block)){
            printf("ERROR: Pointer is not in the heap at line %d\n", lineno);
            exit(1);
        }

        //Make sure the block in the free list is unallocated
        if(get_allocated(curr_free_block) != 0){
            printf("ERROR: Allocated block is in free list at line %d\n", lineno);
            exit(1);
        }

        //Make sure previous and next pointers are consistent
        if(previous_free_block != prev_free_block(curr_free_block)){
            printf("ERROR: Previous and Next pointers are not consistent at line %d\n", lineno);
            exit(1);
        }
        previous_free_block = curr_free_block;
        num_free_blocks_list++;

        dbg_printf("Free pointer: %p\n", curr_free_block);

    }

    //Make sure all unallocated blocks are in the free list
    if(num_free_blocks != num_free_blocks_list){
        printf("ERROR: Free blocks in heap and in list are not consistent at line %d\n", lineno);
        exit(1);
    }


}
