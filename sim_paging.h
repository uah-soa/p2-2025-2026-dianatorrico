/*
    sim_paging.h
*/

#ifndef _SIM_PAGING_H_
#define _SIM_PAGING_H_

// Structure that holds the state of a page,
// sumulating an entry of the page table

typedef struct
{
    char present;       // 1 = loaded in a frame
    int frame;          // Frame where it is loaded
    char modified;      // 1 = must be written back to disc
                            // if moved out of the frame
    // For FIFO 2nd chance
    char referenced;    // 1 = page referenced recently

    // For LRU(t)
    unsigned timestamp; // Time mark of last reference

    // NOTE: The previous two fiels are in this structure
    //       ---and not in sframe--- because they simulate
    //       a mechanism that, in reality, would be
    //       supported by the hardware.
}
spage;

// Structure that holds the state of a frame
// (the hardware doesn't know anything about this struct)

typedef struct
{
    int page;           // Number of the page loaded, if any

    // For managing free frames and for FIFO and FIFO 2nd ch.
    int next;           // Next frame in the list
}
sframe;

// Struture that contains the state of the whole system

typedef struct
{
    // Page table (maintained by HW and OS)
    int pagsz;
    int numpags;
    spage * pgt;
    int lru;               // Only for LRU replacement
    unsigned clock;        // Only for LRU(t) replacement

    // Frames table (maintained by the OS only)
    int numframes;
    sframe * frt;
    int listfree;
    int listoccupied;      // Only for FIFO and FIFO 2nd ch.

    // Trace data
    int numrefsread;       // Counter of read operations
    int numrefswrite;      // Counter of write operations
    int numpagefaults;     // Counter of page faults
    int numpgwriteback;    // Counter of write back (to disc) ops.
    int numillegalrefs;    // References out of range
    char detailed;         // 1 = show step-by-step information
}
ssystem;

// Function that initialises the tables

void init_tables (ssystem * S);

// Functions that simulate the hardware of the MMU

unsigned sim_mmu (ssystem * S, unsigned virt_address, char op);
void reference_page (ssystem * S, int page, char op);

// Functions that simulate the operating system

void handle_page_fault (ssystem * S, unsigned virt_address);
int choose_page_to_be_replaced (ssystem * S);
void replace_page (ssystem * S, int victim, int newpage);
void occupy_free_frame (ssystem * S, int frame, int page);

// Functions that show results

void print_report (ssystem * S);
void print_page_table (ssystem * S);
void print_frames_table (ssystem * S);
void print_replacement_report (ssystem * S);

#endif // _SIM_PAGING_H_

