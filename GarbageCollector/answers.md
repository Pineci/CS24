# Q0
The value_size variable stores the number of bytes needed for a specific instance of value_t. This is necessary for the allocator to know so it can implement a free list correctly, like we did in malloc last week. It is also used in the garbage collector to know how many bytes to copy over from the from space to the to space.

# Q1
TOMBSTONE_REF is used to as a stand in marker in the keys array of a dictionary. It is used to indicate whether a key slot has been deleted, so then it will not be necessary to shift all elements in the keys and values array.

# Q2
Once a value_t is freed, its reference count no longer matters, it is known to be 0. This space can then be used for storing a pointer to the next value_t in the free list. However, value_size is still needed since we still need to be able to traverse the list physically in memory. 
