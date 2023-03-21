/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
                                                                          size_t char_offset, size_t *entry_offset_byte_rtn)
{

    size_t total_count = 0;                // total count of all packets from entry point
    size_t size_entry = 0;                 // size of the current index packet
    uint8_t temp_index = buffer->out_offs; // position of read index
                                           // To check if buffer is empty
    if ((buffer->full == false) && (buffer->in_offs == buffer->out_offs))
    {
        return NULL;
    }
    while (1)
    {
        size_entry = buffer->entry[temp_index].size; // size of current index
        total_count += size_entry;                   // total size
        if (char_offset <= (total_count - 1))        // check if within the total count
        {
            *entry_offset_byte_rtn = (size_entry) - (total_count - char_offset); // check for no of bytes inside current packet
            return &buffer->entry[temp_index];                                   // return the index address
        }
        temp_index++;
        temp_index = temp_index % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);
        if (temp_index == buffer->in_offs) // read until both input and output index coincide
            break;
    }
    return NULL;
}

/**
 * Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
 * If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
 * new start location.
 * Any necessary locking must be handled by the caller
 * Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
 */
const char *aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    const char *ptr = NULL;
    if ((buffer->in_offs == buffer->out_offs) && buffer->full == true) // checking for full condition and overwrite data
    {
        ptr = buffer->entry[buffer->in_offs].buffptr;                                    // store location before over writing
        buffer->entry[buffer->in_offs] = *(add_entry);                                   // adding data by overwriting
        buffer->in_offs++;                                                               // incrementing input position
        buffer->in_offs = (buffer->in_offs) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED); // looping circular buffer
        buffer->out_offs = buffer->in_offs;                                              // will be in same position
    }
    else
    {
        buffer->entry[buffer->in_offs] = *(add_entry);
        buffer->in_offs++;
        buffer->in_offs = (buffer->in_offs) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED); // looping circular buffer
        if (buffer->in_offs == buffer->out_offs)                                         // checking for full condition and setting flag
        {
            buffer->full = true;
        }
        else
            buffer->full = false;
    }
    return ptr;
}

/**
 * Initializes the circular buffer described by @param buffer to an empty struct
 */
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer, 0, sizeof(struct aesd_circular_buffer));
}
