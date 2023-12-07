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

/**
     * An array of pointers to memory allocated for the most recent write operations
     */
    //struct aesd_buffer_entry  entry[AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];
    /**
     * The current location in the entry structure where the next write should
     * be stored.
     */
    //uint8_t in_offs;
    /**
     * The first location in the entry structure to read from
     */
    //uint8_t out_offs;
    /**
     * set to true when the buffer entry structure is full
     */
    //bool full;

    // max operations => AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED



struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{   
    int8_t index = buffer->out_offs;;
    struct aesd_buffer_entry *currententry = NULL;
    int8_t preventrysize = 0;
    int8_t entrytot = 0;
    int8_t maxentrysize = 0;
    int8_t entryoffset = 0;

    //check that pointers are setup properly
    if(buffer == NULL || entry_offset_byte_rtn == NULL){
        return NULL;
    }


    // whille loop used as the AESD_CIRCULAR_BUFFER_H kept failing
    while((index != buffer->out_offs) || (entrytot == 0))
    {   
        //update all variables to current index entry before checking if we found the offset
        currententry = &buffer->entry[index]; 
        maxentrysize = preventrysize  + currententry->size;
        entryoffset = char_offset - preventrysize ;

        //want to make sure the offset is between the maximum possible size with the current entry 
        // and greater then the size of all previous entries
        if( (char_offset < maxentrysize) && (char_offset >= preventrysize ) ){
            
            *entry_offset_byte_rtn = entryoffset;
            return currententry;
  
        }

        // if not found, continue with while loop by increasing total size checked and index value
        if (currententry->buffptr != NULL){
            preventrysize  = preventrysize + currententry->size;
        }
        
        index++;
        if(index >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
            index = 0;
        }
        
        //check that we have gone through at least one loop
        entrytot++;

    }
   
   // return null if we've gone through it all
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    //check that pointers are setup properly
    if(buffer == NULL || add_entry == NULL){
        return;
    }

    //add entry
    buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
    buffer->entry[buffer->in_offs].size = add_entry->size;
    
    //advance pointer, declaring new variable to aid in debugging
    int8_t newsize = buffer->in_offs + 1;

    if(newsize >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        buffer->in_offs = 0;
    }
    else
    {
        buffer->in_offs++;
    }
    

    //check if full and set if true plus reseting out_offs to the new start location specified by in_offs
    if( (buffer->in_offs == buffer->out_offs) || (buffer->full == true) )
    {
        buffer->full = true;

        buffer->out_offs = buffer->in_offs;
    }

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
