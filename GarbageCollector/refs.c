/*! \file
 * Manages references to values allocated in a memory pool.
 * Implements reference counting and garbage collection.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2010.
 * All rights reserved.
 */

#include "refs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "eval.h"
#include "mm.h"

/*! The alignment of value_t structs in the memory pool. */
#define ALIGNMENT 8


//// MODULE-LOCAL STATE ////

/*!
 * The start of the allocated memory pool.
 * Stored so that it can be free()d when the interpreter exits.
 */
static void *pool;

static void *from_space = NULL;
static void *to_space = NULL;
static size_t pool_size = 0;


/*!
 * This is the "reference table", which maps references to value_t pointers.
 * The value at index i is the location of the value_t with reference i.
 * An unused reference is indicated by storing NULL as the value_t*.
 */
static value_t **ref_table;

/*!
 * This is the number of references currently in the table, including unused ones.
 * Valid entries are in the range 0 .. num_refs - 1.
 */
static reference_t num_refs;

/*!
 * This is the maximum size of the ref_table.
 * If the table grows larger, it must be reallocated.
 */
static reference_t max_refs;


//// FUNCTION DEFINITIONS ////


/*!
 * This function initializes the references and the memory pool.
 * It must be called before allocations can be served.
 */
void init_refs(size_t memory_size, void *memory_pool) {
    /* Use the memory pool of the given size.
     * We round the size down to a multiple of ALIGNMENT so that values are aligned.
     */
    pool_size = (memory_size/2) / ALIGNMENT * ALIGNMENT;
    mm_init(pool_size, memory_pool);
    pool = memory_pool;
    from_space = memory_pool;

    to_space = (void*)((char*)memory_pool + (memory_size/2) / ALIGNMENT * ALIGNMENT);

    /* Start out with no references in our reference-table. */
    ref_table = NULL;
    num_refs = 0;
    max_refs = 0;
}


/*! Allocates an available reference in the ref_table. */
static reference_t assign_reference(value_t *value) {
    /* Scan through the reference table to see if we have any unused slots
     * that can store this value. */
    for (reference_t i = 0; i < num_refs; i++) {
        if (ref_table[i] == NULL) {
            ref_table[i] = value;
            return i;
        }
    }

    /* If we are out of slots, increase the size of the reference table. */
    if (num_refs == max_refs) {
        /* Double the size of the reference table, unless it was 0 before. */
        max_refs = max_refs == 0 ? INITIAL_SIZE : max_refs * 2;
        ref_table = realloc(ref_table, sizeof(value_t *[max_refs]));
        if (ref_table == NULL) {
            fprintf(stderr, "could not resize reference table");
            exit(1);
        }
    }

    /* No existing references were unused, so use the next available one. */
    reference_t ref = num_refs;
    num_refs++;
    ref_table[ref] = value;
    return ref;
}


/*! Attempts to allocate a value from the memory pool and assign it a reference. */
reference_t make_ref(value_type_t type, size_t size) {
    /* Force alignment of data size to ALIGNMENT. */
    size = (size + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;

    /* Find a (free) location to store the value. */
    value_t *value = mm_malloc(size);

    /* If there was no space, then fail. */
    if (value == NULL) {
        return NULL_REF;
    }

    /* Initialize the value. */
    assert(value->type == VAL_FREE);
    value->type = type;
    value->ref_count = 1; // this is the first reference to the value

    /* Set the data area to a pattern so that it's easier to debug. */
    memset(value + 1, 0xCC, value->value_size - sizeof(value_t));

    /* Assign a reference_t to it. */
    return assign_reference(value);
}


/*! Dereferences a reference_t into a pointer to the underlying value_t. */
value_t *deref(reference_t ref) {
    /* Make sure the reference is actually a valid index. */
    assert(ref >= 0 && ref < num_refs);

    value_t *value = ref_table[ref];

    /* Make sure the reference's value is within the pool!
     * Also ensure that the value is not NULL, indicating an unused reference. */
    assert(is_pool_address(value));

    return value;
}

/*! Returns the reference that maps to the given value. */
reference_t get_ref(value_t *value) {
    for (reference_t i = 0; i < num_refs; i++) {
        if (ref_table[i] == value) {
            return i;
        }
    }
    assert(!"Value has no reference");
}


/*! Returns the number of values in the memory pool. */
size_t refs_used() {
    size_t values = 0;
    for (reference_t i = 0; i < num_refs; i++) {
        if (ref_table[i] != NULL) {
            values++;
        }
    }
    return values;
}


//// REFERENCE COUNTING ////

/*! Increases the reference count of the value at the given reference. */
void incref(reference_t ref){
    value_t *value = deref(ref);
    value->ref_count++;
}

// Traversal function used in decref to decrease the values of all further
// references. Also used in the garbage collector when deleting garbage
void traverse_decref(value_t *value){
    if(value->type == VAL_LIST){
        decref(((list_value_t*)value)->values);
    } else if(value->type == VAL_DICT){
        decref(((dict_value_t*)value)->keys);
        decref(((dict_value_t*)value)->values);
    } else if(value->type == VAL_REF_ARRAY){
        ref_array_value_t *arr = (ref_array_value_t*)value;
        for(size_t i = 0; i < arr->capacity; i++){
            reference_t ref = arr->values[i];
            if(ref != NULL_REF && ref != TOMBSTONE_REF){
                decref(ref);
            }
        }
    }
}

/*!
 * Decreases the reference count of the value at the given reference.
 * If the reference count reaches 0, the value is definitely garbage and should be freed.
 */
void decref(reference_t ref) {
    if(is_pool_address(ref_table[ref])){
        value_t *value = deref(ref);
        value->ref_count--;
        if(value->ref_count > 0){
            return;
        }
        ref_table[ref] = NULL;
        traverse_decref(value);
        mm_free(value);
    }
}


//// END REFERENCE COUNTING ////


//// GARBAGE COLLECTOR ////

//Copies the reference_t ref to the first available spot in memory in the newly
//initialized pool. Also copies contents of all references contained inside ref
void copy_contents(const char *name, reference_t ref){
    (void)name;
    if(!is_pool_address(ref_table[ref])){
        value_t *old_value = ref_table[ref];
        value_t *value = mm_malloc(old_value->value_size);
        memcpy(value, old_value, old_value->value_size);
        ref_table[ref] = value;
        if(value->type == VAL_LIST){
            list_value_t *list_value = (list_value_t*)value;
            copy_contents(NULL, list_value->values);
        } else if (value->type == VAL_DICT){
            dict_value_t *dict_value = (dict_value_t*)value;
            copy_contents(NULL, dict_value->keys);
            copy_contents(NULL, dict_value->values);
        } else if (value->type == VAL_REF_ARRAY){
            ref_array_value_t *ref_array = (ref_array_value_t*)value;
            for(size_t i = 0; i < ref_array->capacity; i++){
                reference_t ref = ref_array->values[i];
                if(ref != NULL_REF && ref != TOMBSTONE_REF){
                    copy_contents(NULL, ref);
                }
            }
        }
    }
}


void collect_garbage(void) {
    if (interactive) {
        fprintf(stderr, "Collecting garbage.\n");
    }
    size_t old_use = mem_used();

    // Start by initializing to space and swapping the spaces
    mm_init(pool_size, to_space);
    foreach_global(copy_contents);
    void *temp = from_space;
    from_space = to_space;
    to_space = temp;

    //Delete old structures which are now garbage from the ref table
    for(reference_t i = 0; i < num_refs; i++){
        if(!is_pool_address(ref_table[i]) && ref_table[i] != NULL){
            traverse_decref(ref_table[i]);
            ref_table[i] = NULL;
        }
    }

    if (interactive) {
        // This will report how many bytes we were able to free in this garbage
        // collection pass.
        fprintf(stderr, "Reclaimed %zu bytes of garbage.\n", old_use - mem_used());
    }
}



//// END GARBAGE COLLECTOR ////


/*!
 * Clean up the allocator state.
 * This requires freeing the memory pool and the reference table,
 * so that the allocator doesn't leak memory.
 */
void close_refs(void) {
    free(pool);
    free(ref_table);
}
