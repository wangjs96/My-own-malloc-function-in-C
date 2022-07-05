#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "memory_management.h"

//Block header struct declare
struct _block_header
{
    size_t b_size;
    struct _block_header* prev;
    struct _block_header* next;
    void* start_addr;
    unsigned int padding;
    unsigned int free;
    unsigned int number;
    struct _block_header* start_address;
    struct _block_header* end_address;
};

//Declare the first block header
struct _block_header* first_block_header = NULL;

//Change the size required by users to the multiple of 8
size_t padding(size_t size)
{
    if(size % 8 != 0)
    {
        return (size/8+1)*8;
    }

    else
        return size;
}

//Used to allocate memory to users
void *_malloc(size_t size)
{
    //If users require 0 byte memory, the function will return NULL
    if(size == 0)
        return NULL;
    //Ensure the first block header will only be initialized with one time
    if(first_block_header == NULL)
    {
        //Assignment to the first block header with address of current heap top
        first_block_header = (struct _block_header*)sbrk(0);
        //Ask system to allocate memory for the first block header
        if(brk(sbrk(0) + sizeof(struct _block_header))==0)
        {
            //Initialize the values in the first block header
            first_block_header->b_size = 0;
            first_block_header->free = 0;
            first_block_header->number = 0;
            first_block_header->next = NULL;
            first_block_header->prev = NULL;
            first_block_header->start_addr = NULL;
            first_block_header->end_address = NULL;
            first_block_header->start_addr = sbrk(0);
            first_block_header->padding = 0;
        }

    }
    //The acceotable size of memory
    int suitable_size = 0;
    //Pointer used to traverse to avoid changing the value of first block header
    struct _block_header* ptr_to_tra = NULL;
    //Pointer to the last block header
    struct _block_header* ptr_to_last_one = NULL;
    //Call the padding function for make size of memory become multiple of 8
    suitable_size = padding(size);

    //Assign the address of first block header to pointer used to traverse
    ptr_to_tra = first_block_header;
    //Judge whether current block header is the last one
    while(ptr_to_tra != NULL)
    {

        //Judge whether the current memory blocks can satisfy the new memory requirements
        if(ptr_to_tra->free == 1&&ptr_to_tra->b_size >= suitable_size)
        {
            //Record the memory left after splitting of memory block
            size_t memory_remainder = ptr_to_tra->b_size - suitable_size - sizeof(struct _block_header);
            printf("memory remainder:%p\n",ptr_to_tra);
            //Judge whether the remainder of memory left is greater than 0
            if(memory_remainder > 0)
            {
                printf("Suitable current block:%p\n",ptr_to_tra);
                //Change the memory size stored in the old block header
                ptr_to_tra->b_size = suitable_size;
                //Reset the free flag in the old block header
                ptr_to_tra->free = 0;
                //Set the padding value
                ptr_to_tra->padding = suitable_size - size;
                //Create a new block header to store the information of memory block left
                struct _block_header* split_bh = ptr_to_tra->start_addr + ptr_to_tra->b_size;
                //Set the size of new memory block
                split_bh->b_size = padding(memory_remainder);
                //Set the free flag of new memory block as freed
                split_bh->free = 1;
                //Add the new memory block into the link
                if(ptr_to_tra->next == NULL)
                    split_bh->next = NULL;
                else
                    {
                        ptr_to_tra->next->prev = split_bh;
                        split_bh->next = ptr_to_tra->next;
                    }
                //Assignment to the new block header's previous element
                split_bh->prev = ptr_to_tra;
                //Set the value of padding
                split_bh->padding = 0;
                //Set the address of memory block start
                split_bh->start_addr = (void*)split_bh + sizeof(struct _block_header);
                //Connect with the split block header
                ptr_to_tra->next = split_bh;
                //Return the start address of old memory block
                return ptr_to_tra->start_addr;
            }
            else
                //If the memory block freed has the same size with the requirement of user
                return ptr_to_tra->start_addr;
        }

        //Store the last one block header
        ptr_to_last_one = ptr_to_tra;
        //Move to the next block header
        ptr_to_tra = ptr_to_tra->next;
    }

    //Allocate memory for new block header
    if(brk(sbrk(0) + sizeof(struct _block_header)) == 0)
    {

        //Assign memory address to new block header
        struct _block_header* new_bh = ptr_to_last_one->start_addr + ptr_to_last_one->b_size;
        //Set the size of memory block
        new_bh->b_size = suitable_size;
        //Set the state of free
        new_bh->free = 0;
        //Set the address of next block header.
        new_bh->next = NULL;
        //Connect with the current last block header
        new_bh->prev = ptr_to_last_one;
        //Set the value of padding
        new_bh->padding = suitable_size - size;
        //Set the start address of memory address
        new_bh->start_addr = sbrk(0);
        //Connect the new block header with the old last one
        ptr_to_last_one->next = new_bh;
        //Increase the counter of link
        first_block_header->number += 1;
        //Store the start address of memory allocated by malloc function
        if(first_block_header->number == 1)
            first_block_header->start_address = new_bh->start_addr;
        //Return the allocation result as the start address of the memory block
        if(brk(new_bh->start_addr + new_bh->b_size)==0)
        {
            first_block_header->end_address = sbrk(0);
            return new_bh->start_addr;
        }
        else
        {
            //If the memory allocation is not successful, return the exception
            return (void*)0;
        }
    }

}
//Free memory and merge the fragments
void _free(void* ptr)
{
    //Check whether the pointer transmitted is null
    if(ptr != NULL)
    {
        //Calculate the address of block header
        struct _block_header* free_block = ((struct _block_header*)(ptr-sizeof(struct _block_header)));
        //Check the validation of address transmitted
        if(ptr >= (void*)first_block_header->start_address&&ptr <= (void*)first_block_header->end_address)
        {
            //Set the free flag of block header as 1
            free_block->free = 1;
            //Check whether the block header needed to be freed can be merged with other memory blocks
            if(free_block->next != NULL&&(free_block->prev->free == 1||free_block->next->free == 1))
            {

                //If the memory block before the current block is freed, the current memory block can be merged with it
                if(free_block->prev->free == 1)
                {
                    //Change the size information stored in the block header before
                    free_block->prev->b_size += (sizeof(struct _block_header) + free_block->b_size);
                    //Reconnect the link
                    free_block->prev->next = free_block->next;
                    free_block->next->prev = free_block->prev;
                    //Change the value of padding
                    free_block->prev->padding = 0;
                }
                else
                {
                    //Change the size information stored in the current block header
                    free_block->b_size += (sizeof(struct _block_header) + free_block->next->b_size);
                    //Reconnect the link
                    free_block->next->next->prev = free_block;
                    free_block->next = free_block->next->next;
                    //Change the value of padding
                    free_block->padding = 0;
                }

            }
            //If the memory block freed is the last one in the link, the heap top should be moved back to release the memory
            else if(free_block->next == NULL)
            {
                free_block->prev->next = NULL;
                //Move the heap top back
                brk(sbrk(0) - (sizeof(struct _block_header) + free_block->b_size));
                //Check the end of memory block allocated by malloc function
                first_block_header->end_address = sbrk(0);
            }
        }

    }

}


