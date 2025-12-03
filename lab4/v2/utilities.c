#include "tvpns.h"

struct tunneltab forwardtab[NUMSESSIONS];

void initialize_forward_table() {
    memset(forwardtab, 0, sizeof(forwardtab));
}

int find_free_session_slot() {
    for (int i = 0; i < NUMSESSIONS; i++) {
        if (forwardtab[i].sourceaddr == 0) {
            return i;
        }
    }
    return -1;
}
