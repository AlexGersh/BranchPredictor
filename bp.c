/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdbool.h>

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

//-----------STRUCTS-----------//
typedef struct {
    uint32_t tag;
    uint32_t target;
    uint8_t history;
    FSM_ST *fsm_pointer;
} Btb_row_t;

typedef struct {
    unsigned size;            // size of the BTB table [num of rows]
    unsigned history_size;    // register size [bits]
    unsigned tag_size;        // tag size [bits]
    bool usingGlobalHistory;  // if false --> local history
    bool usingGlobalFSMTable; // if false --> local FSM Table
    ShareMode share_mode;
    FSM_ST fsm_init_st;

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

// not finished yet
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
            unsigned fsmState, bool isGlobalHist, bool isGlobalTable,
            int Shared) {
    if (!isBbtbSizeValid(btbSize) || !isHistSizeValid(historySize) ||
        !isTagSizeValid(tagSize, btbSize) || !isFsmInitStateValid(fsmState) ||
        !isSharedValid(Shared))
        return -1;

    // initializing
    my_btb.size = btbSize;
    my_btb.history_size = historySize;
    my_btb.tag_size = tagSize;
    my_btb.fsm_init_st = (FSM_ST)fsmState;
    my_btb.usingGlobalHistory = isGlobalHist;
    my_btb.usingGlobalFSMTable = isGlobalTable;
    my_btb.share_mode = (ShareMode)Shared;

    my_btb.table = malloc(my_btb.size * sizeof(Btb_row_t));
    if (!my_btb.table)
        return -1; // allocation error

    if (isGlobalTable) {
        //
    }

    // else local table
    my_btb.table->fsm_pointer = malloc(sizeof(FSM_ST));

    for (int i = 0; i < my_btb.size; i++) {
        my_btb.table[i].tag = NEW;
    }

    return 0; // success
}

// not finished yet
bool BP_predict(uint32_t pc, uint32_t *dst) { return false; }

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
    bool flag;
    switch (btbSize) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
        flag = true;
    default:
        flag = false;
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