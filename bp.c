/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//-----------DEFINES AND MACROS-----------//
#define NEW -1

#define H_TO_BITS(n)                                                           \
    ((n) == 1    ? 0                                                           \
     : (n) == 2  ? 1                                                           \
     : (n) == 4  ? 2                                                           \
     : (n) == 8  ? 3                                                           \
     : (n) == 16 ? 4                                                           \
     : (n) == 32 ? 5                                                           \
                 : -1) // -1 invalid input

typedef enum { SNT, WNT, WT, ST } FSM_ST; // FSM States
typedef enum { not_using_share, using_share_lsb, using_share_mid } ShareMode;
typedef char *fsm_p;
//-----------STRUCTS-----------//
typedef struct {
    uint32_t tag;
    uint32_t target;
    uint8_t history;
    fsm_p fsm_pointer;
} Btb_row_t;

typedef struct {
    unsigned size;            // size of the BTB table [num of rows]
    unsigned history_size;    // register size [bits]
    unsigned tag_size;        // tag size [bits]
    bool usingGlobalHistory;  // if false --> local history
    bool usingGlobalFSMTable; // if false --> local FSM Table
    ShareMode share_mode;
    FSM_ST fsm_init_st;

    fsm_p predictor_table;
    Btb_row_t *table;

} Btb_t;

//-----------VARIABLES-----------//
Btb_t my_btb;

//-----------HELPER FUNCTIONS-----------//
bool isBbtbSizeValid(unsigned btbSize);
bool isHistSizeValid(unsigned hist_size);
bool isTagSizeValid(unsigned tag_size, unsigned btb_size);
bool isFsmInitStateValid(unsigned fsm_init_state);
bool isSharedValid(int shared);
bool isPCValid(uint32_t);

uint32_t getIProwFromPC(uint32_t);
uint32_t getTagFromPC(uint32_t);
void setBtbRow(Btb_row_t *, uint32_t tag, uint32_t target, uint8_t history,
               fsm_p fsm_pointer);
void printBTB();

// not finished yet
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
            unsigned fsmState, bool isGlobalHist, bool isGlobalTable,
            int Shared) {
    if (!isBbtbSizeValid(btbSize) || !isHistSizeValid(historySize) ||
        !isTagSizeValid(tagSize, btbSize) || !isFsmInitStateValid(fsmState) ||
        !isSharedValid(Shared)) {
        return -1;
    }

    const int predictor_array_size = historySize * sizeof(char);
    // initializing
    my_btb.size = btbSize;
    my_btb.history_size = historySize;
    my_btb.tag_size = tagSize;
    my_btb.fsm_init_st = (FSM_ST)fsmState;
    my_btb.usingGlobalHistory = isGlobalHist;
    my_btb.usingGlobalFSMTable = isGlobalTable;
    my_btb.share_mode = (ShareMode)Shared;

    my_btb.table = (Btb_row_t *)malloc(my_btb.size * sizeof(Btb_row_t));
    if (!my_btb.table) {
        fprintf(stderr, "faild to malloc size of table\n");
        return -1; // allocation error
    }

    my_btb.predictor_table =
        (fsm_p)malloc((isGlobalHist ? 1 : my_btb.size) * predictor_array_size);
    if (!my_btb.predictor_table) {
        fprintf(stderr, "faild to malloc size of table\n");
        return -1; // allocation error
    }
    for (int i = 0; i < sizeof(my_btb.predictor_table); i++) {
        my_btb.predictor_table[i] = my_btb.fsm_init_st;
    }

    for (int i = 0; i < my_btb.size; i++) {

        fsm_p fsm_pointer = my_btb.predictor_table +
                            (isGlobalHist ? 0 : i * predictor_array_size);
        setBtbRow(&my_btb.table[i], NEW, 0, 0, fsm_pointer);
    }

    return 0; // success
}

// not finished yet
bool BP_predict(uint32_t pc, uint32_t *dst) {
    bool prediction = false;
    if (!isPCValid(pc)) {
        fprintf(stderr, "PC is not valid\n");
    }

    *dst = pc + 4;
    uint32_t ip = getIProwFromPC(pc);
    Btb_row_t *row = &my_btb.table[ip];

    // if not new
    if (!(my_btb.table[ip].tag == NEW)) {
        prediction = row->fsm_pointer[row->history];
        *dst = row->target;
    }

    return prediction;
}

// not finished yet
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
    return;
}

// not finished yet
void BP_GetStats(SIM_stats *curStats) {
    //
    return;
}

//-----------FUNC DEFS-----------//

bool isBbtbSizeValid(unsigned btbSize) {
    bool flag = false;
    switch (btbSize) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
        flag = true;
        break;
    }
    return flag;
}

bool isHistSizeValid(unsigned hist_size) {
    if (hist_size > 0 && hist_size < 9)
        return true;
    return false;
}

bool isTagSizeValid(unsigned tag_size, unsigned btb_size) {
    if (tag_size >= 0 && tag_size <= (30 - H_TO_BITS(btb_size)))
        return true;
    return false;
}

bool isFsmInitStateValid(unsigned fsm_init_state) {
    if (fsm_init_state >= 0 && fsm_init_state <= 3)
        return true;
    return false;
}

bool isSharedValid(int shared) {
    if (shared >= 0 && shared <= 2)
        return true;
    return false;
}

uint32_t getIProwFromPC(uint32_t pc) {
    int bits = H_TO_BITS(my_btb.size);

    uint32_t mask = 0x1F;      // maximum mask we will have
    mask = mask >> (5 - bits); // get only the relevant bits
    pc = pc >> 2;
    return mask & pc;
}

uint32_t getTagFromPC(uint32_t pc) {
    int tag_size = my_btb.tag_size;
    int num_end_bits = 2 + H_TO_BITS(my_btb.size); // discard alignment + index

    pc = pc >> num_end_bits; // shift out index & alignment

    // Make a mask with tag_size bits set
    uint32_t mask = (1U << tag_size) - 1;

    return mask & pc;
}

bool isPCValid(uint32_t pc) { return !(pc % 4); }

void setBtbRow(Btb_row_t *btb_row, uint32_t tag, uint32_t target,
               uint8_t history, fsm_p fsm_pointer) {

    btb_row->tag = tag;
    btb_row->target = target;
    btb_row->history = history;
    btb_row->fsm_pointer = fsm_pointer;

    return;
}

void printBTB() {

    printf("BTB Configuration:\n");
    printf("Size: %u\n", my_btb.size);
    printf("History Size: %u bits\n", my_btb.history_size);
    printf("Tag Size: %u bits\n", my_btb.tag_size);
    printf("Using Global History: %s\n",
           my_btb.usingGlobalHistory ? "Yes" : "No");
    printf("Using Global FSM Table: %s\n",
           my_btb.usingGlobalFSMTable ? "Yes" : "No");
    printf("Share Mode: %d\n", my_btb.share_mode);
    printf("Initial FSM State: %d\n", my_btb.fsm_init_st);
    printf("PredictorTable %p\n", my_btb.predictor_table);

    printf("\nBTB Rows:\n");
    Btb_row_t *row = NULL;
    for (int i = 0; i < my_btb.size; ++i) {
        row = &my_btb.table[i];
        printf("Row %u:\n", i);
        printf("  Tag: 0x%x", row->tag);
        printf("  Target: 0x%x", row->target);
        printf("  History: 0x%x", row->history);
        // Optional: print FSM pointer or content, depending on implementation
        printf("  FSM Pointer: %p\n", (void *)row->fsm_pointer);
    }
    printf("----\n");
}