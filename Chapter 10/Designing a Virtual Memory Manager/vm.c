/*
    Title: Virtual Memory Manager
    Author: @cardozoguilherme
    Date: 2024-05-22

    Description:
    This project consists of writing a program that translates logical to physical addresses for a virtual address space of size 2^16 = 65,536 bytes. The program
    will read from a file containing logical addresses and, using a TLB and a page table, will translate each logical address to its corresponding physical address
    and output the value of the byte stored at the translated physical address.

    The learning goal is to use simulation to understand the steps involved in translating logical to physical addresses. This will include resolving page faults
    using demand paging, managing a TLB, and implementing a page-replacement algorithm.

    Notes:
    The exercise corresponds to the implementation of Designing a Virtual Memory Manager presented on page P-51 of the book Operating System Concepts, Silberschatz,
    A. et al, 10th edition. However, some modifications follow:

        - The implementation is one in which the physical memory has only 128 frames;
        - Two page replacement algorithms have been implemented, namely fifo and lru, while only fifo will be used in TLB.

    Example Usage:
        make && ./vm address.txt fifo
        make && ./vm address.txt lru
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ADDRESS_LENGTH 10
#define PAGE_NUMBER_BITS 8
#define OFFSET_BITS 8
#define PHYSICAL_MEMORY_FRAMES 128
#define NUMBER_OF_PAGES 256
#define PAGE_SIZE 256
#define FRAME_SIZE PAGE_SIZE
#define TLB_SIZE 16
#define INT_MAX 2147483647

int page_table[NUMBER_OF_PAGES];

typedef struct Address
{
    int virtual_address;
    int page_number;
    int offset;
    struct Address *next;
} Address;

typedef struct Frame
{
    char data[PAGE_SIZE];
    int frame_number;
    int last_used;
    struct Frame *next;
} Frame;

typedef struct TLB_elements
{
    int page_number;
    int frame_number;
} TLB_elements;

TLB_elements TLB[TLB_SIZE];

int page_fault_counter = 0;
int next_victim_frame = 0;
int total_translated_addresses = 0;
int tlb_hit_counter = 0;
int next_tlb_entry = 0;
int current_time = 0;

void insert_address(Address **address_head, int virtual_address, int page_number, int offset)
{
    Address *new = (Address *)malloc(sizeof(Address));
    if (new != NULL)
    {
        new->virtual_address = virtual_address;
        new->page_number = page_number;
        new->offset = offset;
        new->next = NULL;

        if (*address_head == NULL)
        {
            *address_head = new;
        }
        else
        {
            Address *temp = *address_head;
            while (temp->next != NULL)
            {
                temp = temp->next;
            }
            temp->next = new;
        }
    }
}

void create_frame(Frame **frame_head, char data[], int frame_number)
{
    Frame *new = (Frame *)malloc(sizeof(Frame));
    if (new != NULL)
    {
        memcpy(new->data, data, PAGE_SIZE * sizeof(char));
        new->frame_number = frame_number;
        new->last_used = 0;
        new->next = NULL;

        if (*frame_head == NULL)
        {
            *frame_head = new;
        }
        else
        {
            Frame *temp = *frame_head;
            while (temp->next != NULL)
            {
                temp = temp->next;
            }
            temp->next = new;
        }
    }
}

void init_physical_memory(Frame **head)
{
    char initial_data[PAGE_SIZE] = {0};
    for (int i = 0; i < PHYSICAL_MEMORY_FRAMES; i++)
    {
        create_frame(head, initial_data, i);
    }
}

void init_page_table()
{
    for (int i = 0; i < NUMBER_OF_PAGES; i++)
    {
        page_table[i] = -1;
    }
}

void init_tlb()
{
    for (int i = 0; i < TLB_SIZE; i++)
    {
        TLB[i].page_number = -1;
        TLB[i].frame_number = -1;
    }
}

int select_victim_frame_fifo()
{
    if (next_victim_frame >= PHYSICAL_MEMORY_FRAMES)
    {
        next_victim_frame = 0;
    }
    return next_victim_frame++;
}

int select_victim_frame_lru(Frame *head)
{
    Frame *current_frame = head;
    Frame *lru_frame = head;
    int min_last_used = INT_MAX;

    while (current_frame != NULL)
    {
        if (current_frame->last_used < min_last_used)
        {
            min_last_used = current_frame->last_used;
            lru_frame = current_frame;
        }
        current_frame = current_frame->next;
    }

    return lru_frame->frame_number;
}

void load_page_into_frame(Frame *head, int victim_frame, int page_number)
{
    FILE *backing_store = fopen("BACKING_STORE.bin", "rb");
    if (backing_store == NULL)
    {
        printf("Error: could not open backing storage file\n");
        return;
    }

    Frame *current_frame = head;
    int previous_page = -1;

    while (current_frame != NULL)
    {
        if (current_frame->frame_number == victim_frame)
        {
            fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);
            fread(current_frame->data, sizeof(char), PAGE_SIZE, backing_store);
            current_frame->last_used = current_time++;

            for (int i = 0; i < NUMBER_OF_PAGES; i++)
            {
                if (page_table[i] == victim_frame)
                {
                    previous_page = i;
                    break;
                }
            }
            if (previous_page != -1)
            {
                page_table[previous_page] = -1;
            }
            page_table[page_number] = victim_frame;
            break;
        }
        current_frame = current_frame->next;
    }

    fclose(backing_store);
}

void handle_page_fault(Frame *frame_head, int page_number, char *replacement_algorithm)
{
    int victim_frame;

    if (strcmp(replacement_algorithm, "fifo") == 0)
    {
        victim_frame = select_victim_frame_fifo();
    }
    else if (strcmp(replacement_algorithm, "lru") == 0)
    {
        victim_frame = select_victim_frame_lru(frame_head);
    }
    else
    {
        printf("Error: unknown replacement algorithm\n");
        return;
    }
    load_page_into_frame(frame_head, victim_frame, page_number);
    page_table[page_number] = victim_frame;
}

int physical_address_calculator(int frame_number, int offset)
{
    return (frame_number << OFFSET_BITS) | offset;
}

int value_calculator(Frame *frame_head, int frame_number, int offset)
{
    int value;

    while (frame_head != NULL)
    {
        if (frame_head->frame_number == frame_number)
        {
            value = (signed char)frame_head->data[offset];
            break;
        }
        frame_head = frame_head->next;
    }
    return value;
}

void update_tlb(int page_number, int frame_number, int tlb_entry)
{
    TLB[tlb_entry].page_number = page_number;
    TLB[tlb_entry].frame_number = frame_number;
}

void check_tlb(Address *address_head, Frame *frame_head, FILE *output_file, char *replacement_algorithm)
{
    Address *current_address = address_head;
    int page_number_to_check;

    while (current_address != NULL)
    {
        page_number_to_check = current_address->page_number;
        int tlb_hit = 0;
        int tlb_index = -1;
        int frame_number = -1;

        for (int i = 0; i < TLB_SIZE; i++)
        {
            if (TLB[i].page_number == page_number_to_check)
            {
                tlb_hit = 1;
                tlb_hit_counter++;
                frame_number = TLB[i].frame_number;
                tlb_index = i;
                break;
            }
        }

        if (tlb_hit == 1)
        {
            int offset = current_address->offset;
            int physical_address = physical_address_calculator(frame_number, offset);
            int value = value_calculator(frame_head, frame_number, offset);
            fprintf(output_file, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", current_address->virtual_address, tlb_index, physical_address, value);

            Frame *current_frame = frame_head;
            while (current_frame != NULL)
            {
                if (current_frame->frame_number == frame_number)
                {
                    current_frame->last_used = current_time++;
                    break;
                }
                current_frame = current_frame->next;
            }
        }
        else
        {
            if (page_table[page_number_to_check] < 0)
            {
                page_fault_counter++;
                handle_page_fault(frame_head, page_number_to_check, replacement_algorithm);
            }

            frame_number = page_table[page_number_to_check];
            int offset = current_address->offset;
            int physical_address = physical_address_calculator(frame_number, offset);
            int value = value_calculator(frame_head, frame_number, offset);
            fprintf(output_file, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", current_address->virtual_address, next_tlb_entry, physical_address, value);

            update_tlb(page_number_to_check, frame_number, next_tlb_entry);
            next_tlb_entry = (next_tlb_entry + 1) % TLB_SIZE;

            Frame *current_frame = frame_head;
            while (current_frame != NULL)
            {
                if (current_frame->frame_number == frame_number)
                {
                    current_frame->last_used = current_time++;
                    break;
                }
                current_frame = current_frame->next;
            }
        }
        current_address = current_address->next;
    }
}

void extract_page_number_and_offset(Address **head, char *address_file)
{
    FILE *addresses_file = fopen(address_file, "r");
    int virtual_address;

    if (addresses_file == NULL)
    {
        printf("Error: could not open addresses file\n");
        return;
    }

    char address_line[MAX_ADDRESS_LENGTH];

    while (fgets(address_line, sizeof(address_line), addresses_file))
    {
        virtual_address = atoi(address_line);
        int page_number = (virtual_address >> 8) & 0xFF;
        int offset = virtual_address & 0xFF;
        total_translated_addresses++;
        insert_address(head, virtual_address, page_number, offset);
    }

    fclose(addresses_file);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <address_file> <replacement_algorithm>\n", argv[0]);
        return 1;
    }

    char *address_file = argv[1];
    char *replacement_algorithm = argv[2];

    Address *address_head = NULL;
    Frame *frame_head = NULL;

    FILE *output_file = fopen("correct.txt", "w");
    if (output_file == NULL)
    {
        printf("Error: could not write output file\n");
        return 0;
    }

    init_page_table();
    init_physical_memory(&frame_head);
    init_tlb();
    extract_page_number_and_offset(&address_head, address_file);
    check_tlb(address_head, frame_head, output_file, replacement_algorithm);

    fprintf(output_file, "Number of Translated Addresses = %d\n", total_translated_addresses);
    fprintf(output_file, "Page Faults = %d\n", page_fault_counter);
    fprintf(output_file, "Page Fault Rate = %.3f\n", (float)page_fault_counter / total_translated_addresses);
    fprintf(output_file, "TLB Hits = %d\n", tlb_hit_counter);
    fprintf(output_file, "TLB Hit Rate = %.3f\n", (float)tlb_hit_counter / total_translated_addresses);

    while (address_head != NULL)
    {
        Address *temp = address_head;
        address_head = address_head->next;
        free(temp);
    }

    while (frame_head != NULL)
    {
        Frame *temp = frame_head;
        frame_head = frame_head->next;
        free(temp);
    }

    fclose(output_file);
    return 0;
}
