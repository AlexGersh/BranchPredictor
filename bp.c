/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//-----------DEFINES AND MACROS-----------//
#define NEW -1

/* we need to save to btb table only 30 bits and not 32, because we know that
   every address is 32 bits and has 2 zeros at the end for alignment.
   So we will save only 30 bits of target at BTB table */
#define TARGET_BITS_COUNT 30

// function return h.
//   h=sqrt(n)
#define H_TO_BITS(n)                                                           \
    ((n) == 1    ? 0                                                           \
     : (n) == 2  ? 1                                                           \
     : (n) == 4  ? 2                                                           \
     : (n) == 8  ? 3                                                           \
     : (n) == 16 ? 4                                                           \
     : (n) == 32 ? 5                                                           \
                 : -1) // -1 invalid input
#define POW_2(n)                                                               \
    ((n) == 0   ? 1                                                            \
     : (n) == 1 ? 2                                                            \
     : (n) == 2 ? 4                                                            \
     : (n) == 3 ? 8                                                            \
     : (n) == 4 ? 16                                                           \
     : (n) == 5 ? 32                                                           \
     : (n) == 6 ? 64                                                           \
     : (n) == 7 ? 128                                                          \
     : (n) == 8 ? 256                                                          \
                : -1)                     //-1 invalid input
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
    FSM_ST fsm_init_st; // how to init all bimodal predictors

    fsm_p predictor_table; // table of all predictors
    unsigned predictor_table_size;
    Btb_row_t *table; // BTB table

    SIM_stats status;

} Btb_t;

//-----------VARIABLES-----------//
Btb_t my_btb;

//-----------HELPER FUNCTIONS-----------//

// function check if btbSize is valid
bool isBbtbSizeValid(unsigned btbSize);

// function check is history size is valid
bool isHistSizeValid(unsigned hist_size);

// function check if tag size is valid.
bool isTagSizeValid(unsigned tag_size, unsigned btb_size);

// function check if the inital fsm state is valid
bool isFsmInitStateValid(unsigned fsm_init_state);

// function check if shared_value_state is valid
bool isSharedValid(int shared);

// fucntion check if pc valid
bool isPCValid(uint32_t);

// function calculate the IP(row index) of PC address.
//@param: uint32_t - valid pc address
// return - uint32_t - IP
uint32_t getIProwFromPC(uint32_t);

// function calculate the TAG of PC address.
//@param: uint32_t - valid pc address.
// return - uint32_t - TAG
uint32_t getTagFromPC(uint32_t);

// function gets pointer to row in the BTB. the function init the row
// function will override exsisting row and his (if any) local predictors
//@params: BTB_row_t* row - row in the btb
//                     others - valid params to set the row.
void setBtbRow(Btb_row_t *, uint32_t tag, uint32_t target, uint8_t history,
               fsm_p fsm_pointer);

// function print BTB. for debugging- if you read this we forgot to delete this
void printBTB();

// function update the predictor of given row according to if branch is taken or
// not
void updatePredictor(Btb_row_t *row, uint32_t ip, bool taken);

//function calculate the index of fsm relative to its fsm_pointer.
//in calulation take in a count if using_shard_lsb or using_shard_mid active
uint8_t getIndexFSM(Btb_row_t*,uint32_t);
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
            unsigned fsmState, bool isGlobalHist, bool isGlobalTable,
            int Shared) {
    if (!isBbtbSizeValid(btbSize) || !isHistSizeValid(historySize) ||
        !isTagSizeValid(tagSize, btbSize) || !isFsmInitStateValid(fsmState) ||
        !isSharedValid(Shared)) {
        return -1;
    }

    const int predictor_array_size = POW_2(historySize) * sizeof(char);
    // initializing
    my_btb.size = btbSize;
    my_btb.history_size = historySize;
    my_btb.tag_size = tagSize;
    my_btb.fsm_init_st = (FSM_ST)fsmState;
    my_btb.usingGlobalHistory = isGlobalHist ? 1 : 0;
    my_btb.usingGlobalFSMTable = isGlobalTable;
    my_btb.share_mode = (ShareMode)Shared;
    my_btb.status = (SIM_stats){0, 0, 0}; // init stats to 0
    // BTB_SIZE=btbSize*Row_SIZE
    my_btb.table = (Btb_row_t *)malloc(my_btb.size * sizeof(Btb_row_t));

    if (!my_btb.table) {
        fprintf(stderr, "faild to malloc size of table\n");
        return -1; // allocation error
    }

    /* if Global history:
        predictor array is shared and predictor_arr_size = historySize*fsm_size
       if local history
        predictor array is not shared and
        predictor_arr_size=historySize*fsm_size*Num_of_BTB_rows*/
    my_btb.predictor_table_size =
        (isGlobalHist ? 1 : my_btb.size) * predictor_array_size;
    my_btb.predictor_table = (fsm_p)malloc(my_btb.predictor_table_size);

    if (!my_btb.predictor_table) {
        fprintf(stderr, "faild to malloc size of table\n");
        return -1; // allocation error
    }

    // init the predictors fsm
    for (int i = 0; i < my_btb.predictor_table_size; i++) {
        my_btb.predictor_table[i] = my_btb.fsm_init_st;
    }

    for (int i = 0; i < my_btb.size; i++) {
        fsm_p fsm_pointer = my_btb.predictor_table +
                            (isGlobalHist ? 0 : i * predictor_array_size);
        setBtbRow(&my_btb.table[i], NEW, 0, 0, fsm_pointer);
    }

    // init status

    my_btb.status.size =
        my_btb.size *
            (1 +               // valid bit of every branch on the table
             my_btb.tag_size + // tag size
             (isGlobalTable
                  ? 0
                  : my_btb.history_size) // if globalTable we dont multiply
             + TARGET_BITS_COUNT)
        // if globalTable we count history size only once
        + (isGlobalTable ? my_btb.history_size : 0) // history size
        // predictor table
        + (isGlobalHist ? 1 : my_btb.size) * POW_2(my_btb.history_size) * 2;

    return 0; // success
}

bool BP_predict(uint32_t pc, uint32_t *dst) {

    // DEBUG
    DEBUG_COMMAND(printf("BP_predict: pc=0x%x ", pc));
    bool prediction = false;
    if (!isPCValid(pc)) {
        fprintf(stderr, "PC is not valid\n");
        return -1;
    }

    *dst = pc + 4;
    uint32_t ip = getIProwFromPC(pc);
    uint32_t tag = getTagFromPC(pc);
    Btb_row_t *row = &my_btb.table[ip];

    uint8_t index = getIndexFSM(row, pc);

    // if not new
    if (!(my_btb.table[ip].tag == NEW) && row->tag == tag) {
        DEBUG_COMMAND(printf("FSM=%d\n", row->fsm_pointer[index]);)
        prediction = (row->fsm_pointer[index] < 2 ? false : true);
        if (prediction)
            *dst = row->target;
    }
    // DEBUG
    DEBUG_COMMAND(
        printf("*dst=0x%x prediction=%s\n", *dst, (prediction ? "T" : "N"));)

    return prediction;
}

// not finished yet
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {

    DEBUG_COMMAND(
        printf("BP_update : pc=0x%x targetPc=0x%x taken=%s pred_dst=0x%x \n",
               pc, targetPc, (taken ? "T" : "N"), pred_dst););
    if (!isPCValid(pc)) {
        fprintf(stderr, "PC is not valid\n");
        return;
    }

    uint32_t ip = getIProwFromPC(pc);
    uint32_t tag = getTagFromPC(pc);
    uint8_t new_history = 0;
    Btb_row_t *row = &my_btb.table[ip];

    // printf("DEBUG : 0x%X\n",targetPc);
    // existing btb
    if (!(my_btb.table[ip].tag == NEW) && row->tag == tag) {
        updatePredictor(row, ip, taken);
    } else { // we add new pc to btb
        setBtbRow(row, tag, targetPc, new_history, row->fsm_pointer);
        updatePredictor(row, ip, taken);
    }

    if(my_btb.usingGlobalHistory){
        for(int i=0;i<my_btb.size;i++)
        {
        //row->history = (row->history << 1) | (taken ? 1 : 0);
            (my_btb.table[i]).history=(row->history << 1) | (taken ?1 :0);
        }
    } else {
        row->history = (row->history << 1) | (taken ? 1 : 0);
    }
    // status update
    my_btb.status.br_num++;
    // quite simple solve no need to explain
    if ((targetPc == pred_dst) ^ taken) {
        my_btb.status.flush_num++;
    }
    // flush happens when (dst==target and taken==0) or (dst!=target and
    // taken==1)
}

// not finished yet
void BP_GetStats(SIM_stats *curStats) {
    curStats->br_num = my_btb.status.br_num;
    curStats->flush_num = my_btb.status.flush_num;
    curStats->size = my_btb.status.size;
    
    //freeing all memory
    free(my_btb.table);
    free(my_btb.predictor_table);
    
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
    if(!my_btb.usingGlobalHistory)
        btb_row->history = history;
    btb_row->fsm_pointer = fsm_pointer;

    for (int i = 0; i < POW_2(my_btb.history_size) * sizeof(char) *
                            !my_btb.usingGlobalHistory;
         i++) {
        btb_row->fsm_pointer[i] = my_btb.fsm_init_st;
    }
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

    printf("\n preidctor table\n");
    int j=0;
    for (int i = 0; i < my_btb.predictor_table_size; i++) {
        if(j++==POW_2(my_btb.history_size)){
            j=0;
            printf("\n");
        }
        printf("FSM=%d ", my_btb.predictor_table[i]);
    }
    printf("\n------------------------------------------------------\n");
}

void updatePredictor(Btb_row_t *row, uint32_t ip, bool taken) {
    int history_masked = ((1U << my_btb.history_size) - 1) & row->history;
    fsm_p fsm_addr = row->fsm_pointer + history_masked;

    if (taken) {
        if (*fsm_addr < ST)
            (*fsm_addr)++;
    } else { // not taken
        if (*fsm_addr > SNT)
            (*fsm_addr)--;
    }
}

uint8_t getIndexFSM(Btb_row_t *row, uint32_t pc) {
    // helper var for share mode
    ShareMode shared_value = my_btb.share_mode;
    uint8_t mask = (1U << my_btb.history_size) - 1;

    // default case when not_sharing
    uint8_t index_masked = mask & row->history;

    // prepating index of predict table
    switch (shared_value) {
    case using_share_lsb:
        pc >>= 2;
        break;
    case using_share_mid:
        pc >>= 16;
        break;
    }
    pc &= mask;
    return index_masked ^= pc;
}