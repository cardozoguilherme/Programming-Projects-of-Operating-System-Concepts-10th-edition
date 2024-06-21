# Virtual Memory Manager

This project consists of writing a program that translates logical to physical addresses for a virtual address space of size 2^16 = 65,536 bytes. The program will read from a file containing logical addresses and, using a TLB and a page table, will translate each logical address to its corresponding physical address and output the value of the byte stored at the translated physical address. 

The learning goal is to use simulation to understand the steps involved in translating logical to physical addresses. This will include resolving page faults using demand paging, managing a TLB, and implementing a page-replacement algorithm. 

This exercise corresponds to the implementation of Designing a Virtual Memory Manager presented on page P-51 of the book Operating System Concepts, Silberschatz, A. et al, 10th edition. However, some modifications follow: 
- The implementation is one in which the physical memory has only 128 frames; 
- Two page replacement algorithms have been implemented, namely fifo and lru, while only fifo will be used in TLB.

## Specifics
- 2^8 entries in the page table
- Page size of 2^8 bytes
- 16 entries in the TLB
- Frame size of 2^8 bytes
- 128 frames
- Physical memory of 32,768 bytes (128 frames × 256-byte frame size)
