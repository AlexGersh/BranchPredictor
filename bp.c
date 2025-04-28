/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

//-----------DEFINES AND MACROS-----------//
#define H_TO_BITS(n) \
	((n)==1  ? 0, : \
	 (n)==2  ? 1, : \
	 (n)==4  ? 2, : \
	 (n)==8  ? 3, : \
	 (n)==16 ? 4 ,: \
	 (n)==32 ? 5, : -1) // -1 invalid input
	 
	 
//-----------VARIABLES-----------//

//-----------structs-----------//

//-----------helper functions-----------//

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

