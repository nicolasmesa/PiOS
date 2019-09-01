#include "mm.h"

// Holds references to the memory pages.
static unsigned short mem_map[PAGING_PAGES] = {
    0,
};

// Returns an address to a free page.
unsigned long get_free_page() {
    // Iterate through all pages until we find one that is free. At that point,
    // we take it and calculate the offset of that page.
    for (int i = 0; i < PAGING_PAGES; i++) {
        if (mem_map[i] == 0) {
            mem_map[i] = 1;
            // We start at LOW_MEMORY and use the index as an offset of the page
            // size.
            return LOW_MEMORY + i * PAGE_SIZE;
        }
    }
    return 0;
}

void free_page(unsigned long p) {
    // p = LOW_MEMORY + i * PAGE_SIZE
    // (p - LOW_MEMORY) = i * PAGE_SIZE
    // (p - LOW_MEMORY)/PAGE_SIZE = i
    mem_map[(p - LOW_MEMORY) / PAGE_SIZE] = 0;
}