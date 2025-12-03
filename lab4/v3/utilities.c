#include "tvpns.h"

struct tunneltab forwardtab[NUMSESSIONS];

void initialize_forward_table() {
    memset(forwardtab, 0, sizeof(forwardtab));
}

int find_free_session_slot() {
    for (int i = 0; i < NUMSESSIONS; i++) {
        // Check if sourceaddr is all zeros (unused entry)
        int is_zero = 1;
        for (int j = 0; j < 16; j++) {
            if (forwardtab[i].sourceaddr.s6_addr[j] != 0) {
                is_zero = 0;
                break;
            }
        }
        if (is_zero) {
            return i;
        }
    }
    return -1; // No free slot found
}
