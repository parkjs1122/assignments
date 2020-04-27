#include "cpu.h"
#include "stages.h"

#include <stdio.h>
// the three operands are encoded inside a single uint16_t. This method cracks them open
static void inline decode_ops(uint16_t input, byte *dest, byte *src1, byte *src2)
{
	*dest = input & 0x001F;
	*src1 = (input & 0x03E0) >> 5;
	*src2 = (input & 0x7C00) >> 10;
}

static void inline jump_to(uint32_t* pc, uint32_t immediate) {
	if (immediate < 0x00400000)
		*pc = 0x00400000 + immediate; // load the target into the PC.
	else
		*pc = immediate;
}

// Function to get branch jump target address with immediate
static uint32_t inline get_jump_addr(uint32_t immediate) {
	if (immediate < 0x00400000)
		return 0x00400000 + immediate; // load the target into the PC.
	else
		return immediate;
}

/**
The execute implementations. These actually perform the action of the stage.
**/

// IF Stage ---------------------------------------------------------------
// Instructions are fetched from memory in this stage, and passed into the CPU's ID latch
void InstructionFetchStage::Execute()
{
	// Input of the program
	int type_branch_predictor = core->type_branch_predictor; // Type of branch predictor (1: 1-bit, 2: 2-bit)
	int num_btb_entries = core->num_btb_entries; // The number of BTB entries
												 //---------------------

												 // On first run, saving options into static variables
	if (PREDICTOR_TYPE == 0) PREDICTOR_TYPE = type_branch_predictor;
	if (BTB_ENTRIES == 0) BTB_ENTRIES = num_btb_entries;

	// Saving whether to branch, index of buffer in BTB and branch target address
	int predictionYN = 0, btb_idx = btb_exist(core->PC);
	uint32_t prediction_addr = 0;

	// If corresponding buffer exists in BTB, bring branch target address from BTB
	if (btb_idx > -1) {
		if (PREDICTOR_TYPE == 1) { // In case of 1-bit predictor
			if (branch_target_buffer[btb_idx].prediction == 1) { // Only if prediction bits is 1
				predictionYN = 1;
				prediction_addr = branch_target_buffer[btb_idx].branch_target_addr;
			}
		}
		else if (PREDICTOR_TYPE == 2) { // In case of 2-bit predictor
			if (branch_target_buffer[btb_idx].prediction >= 2) { // Only if prediction bits is greater than 2
				predictionYN = 1;
				prediction_addr = branch_target_buffer[btb_idx].branch_target_addr;
			}
		}
	}

	right.opcode = core->mem->get<byte>(core->PC);
	uint16_t operands = core->mem->get<uint16_t>(core->PC + 1);
	decode_ops(operands, &right.Rdest, &right.Rsrc1, &right.Rsrc2);
	right.immediate = core->mem->get<uint32_t>(core->PC + 3);
	right.instruction_addr = core->PC; // Save branch instruction address for adding new buffer into BTB
	if (predictionYN) core->PC = prediction_addr; // In case of branch prediction
	else core->PC += 8; // In case of no branch prediction
	right.target_addr = core->PC; // Save branch target address for adding new buffer into BTB
	right.PC = core->PC;

	// Branch prediction ----------------------------------------------------
	// Static branch prediction (Always Not Taken)
	if (predictionYN) right.predict_taken = true; // In case of branch prediction, set predictiong taken TRUE
	else right.predict_taken = false; // In case of no branch prediction, set predictiong taken FALSE
									  // ----------------------------------------------------------------------
}


// ID Stage ---------------------------------------------------------------
void InstructionDecodeStage::Execute()
{
	// Input of the program
	int type_branch_predictor = core->type_branch_predictor; // Type of branch predictor (1: 1-bit, 2: 2-bit)
	int num_btb_entries = core->num_btb_entries; // The number of BTB entries
												 //---------------------

	core->registers[0] = 0; // wire register 0 to zero for all register reads
	right.Rsrc1Val = *(int32_t *)&core->registers[left.Rsrc1];
	right.Rsrc2Val = *(int32_t *)&core->registers[left.Rsrc2];
	right.immediate = left.immediate;
	right.Rsrc1 = left.Rsrc1;
	right.Rsrc2 = left.Rsrc2;
	right.Rdest = left.Rdest;
	right.PC = left.PC;
	right.opcode = left.opcode;
	right.predict_taken = left.predict_taken;
	right.instruction_addr = left.instruction_addr;
	right.target_addr = left.target_addr;
}


// EXE Stage --------------------------------------------------------------
void ExecuteStage::Execute()
{
	// Inputs ---------------------------------------------------------------
	// Values transmitted from left
	// * left.opcode
	// * left.Rsrc1Val
	// * left.Rsrc2Val
	// * left.immediate
	// * left.PC
	// * left.predict_taken

	// Decoded instruction (decoded_inst)
	const instruction* decoded_inst;
	decoded_inst = left.control();

	// Source value (svalue)
	int32_t svalue = left.Rsrc1Val;

	// Source parameter (sparam)
	uint32_t param;
	switch (decoded_inst->alu_source) {
	case 0: // source from register
		param = left.Rsrc2Val;
		break;

	case 1: // immediate add/sub
		param = left.immediate;
		break;

	case 2: // address calculation
		param = left.immediate + *(uint32_t *)&left.Rsrc1Val;
		break;
	}
	int32_t sparam = *(int32_t *)&param;

	// Input of the program
	int type_branch_predictor = core->type_branch_predictor; // Type of branch predictor (1: 1-bit, 2: 2-bit)
	int num_btb_entries = core->num_btb_entries; // The number of BTB entries
												 //---------------------

												 // ----------------------------------------------------------------------

												 // Result (result)
	int32_t result = sparam;

	// ALU Execution --------------------------------------------------------
	switch (decoded_inst->alu_operation) {
	case 0:
		// do nothing, this operation does not require an alu op (copy forward)
		break;

	case 1:
		// do a signed add of reg1 to param
		result = svalue + sparam;
		break;

	case 2:
		// do a signed subtract of param from reg1
		result = svalue - sparam;
		break;
	}
	// ----------------------------------------------------------------------

	// Branch Execution -----------------------------------------------------
	bool taken = 0;
	if (decoded_inst->branch) {
		core->branchcount += 1;
		bool taken = (left.opcode == 2 && left.Rsrc1Val == 0) ||
			(left.opcode == 3 && left.Rsrc1Val >= left.Rsrc2Val) ||
			(left.opcode == 4 && left.Rsrc1Val != left.Rsrc2Val);
		// if mispredict, nop out IF and ID. (mispredict == prediction and taken differ)
		if (core->verbose) {
			if (taken == 1) {
				printf("*** Branch taken: %d %d\n", left.Rsrc1Val, left.Rsrc2Val);
			}
			else {
				printf("*** Branch not taken: %d %d\n", left.Rsrc1Val, left.Rsrc2Val);
			}
			printf("BRANCH COUNT : %d / TYPE : %d / BTB_COUNT : %d / ENTRIES : %d\n", core->branchcount, PREDICTOR_TYPE, BTB_COUNT, BTB_ENTRIES); // For logging
		}
		if (taken == 1 && left.predict_taken == false) { // In case of branch taken and no prediction taken -> Misprediction
			if (core->verbose) {
				printf("*** MISPREDICT!\n");
			}
			core->misscount += 1; // Increasing misprediction count
			btb_add(left.instruction_addr, get_jump_addr(left.immediate)); // Adding new buffer into BTB
			core->ifs.make_nop(); // Clear IF stage
			core->ids.make_nop(); // Clear ID stage
			jump_to(&core->PC, left.immediate); // Jump to branch target address
		}
		else if (taken == 1 && left.predict_taken == true) { // In case of branch taken and prediction taken -> Prediction OK
			if (core->verbose) {
				printf("*** PREDICT OK!\n");
			}
			btb_add(left.instruction_addr, left.target_addr); // Increasing prediction bits
		}
		else if (taken == 0 && left.predict_taken == true) { // In case of no branch taken and prediction taken -> Misprediction
			if (core->verbose) {
				printf("*** MISPREDICT!\n");
			}
			core->misscount += 1; // Increasing misprediction count
			btb_delete(left.instruction_addr); // Deleting buffer or decreasing prediction bits
			core->ifs.make_nop(); // Clear IF stage
			core->ids.make_nop(); // Clear ID stage
			jump_to(&core->PC, left.instruction_addr + 8); // Jump to next address of branch instruction
		}
	}
	// ----------------------------------------------------------------------

	right.aluresult = *(uint32_t *)&result;

	// Copy forward from previous latch -------------------------------------
	right.opcode = left.opcode;
	right.Rsrc1 = left.Rsrc1;
	right.Rsrc2 = left.Rsrc2;
	right.Rsrc1Val = left.Rsrc1Val;
	right.Rsrc2Val = left.Rsrc2Val;
	right.Rdest = left.Rdest;
	// ----------------------------------------------------------------------
}

// MEM Stage --------------------------------------------------------------

void MemoryStage::Execute()
{
	const instruction *control = left.control();

	right.aluresult = left.aluresult;
	right.mem_data = 0;
	if (control->mem_read) {
		if (control->mem_read == 1) {
			right.mem_data = core->mem->get<byte>(left.aluresult);
		}
		else if (control->mem_read == 4) {
			right.mem_data = core->mem->get<uint32_t>(left.aluresult);
		}
	}
	else if (control->mem_write) {
		if (control->mem_write == 1) {
			core->mem->set<byte>(left.aluresult, left.Rsrc2Val);
		}
		else if (control->mem_write == 4) {
			core->mem->set<uint32_t>(left.aluresult, left.Rsrc2Val);
		}
	}
	right.opcode = left.opcode;
	right.Rsrc1Val = left.Rsrc1Val;
	right.Rsrc2Val = left.Rsrc2Val;
	right.Rdest = left.Rdest;
}


// WB Stage ---------------------------------------------------------------
void WriteBackStage::Execute()
{
	const instruction *control = left.control();

	if (control->special_case != NULL) {
		control->special_case(core);
	}
	if (control->register_write) {
		if (control->mem_to_register) {
			// write the mem_data into register Rdest
			core->registers[left.Rdest] = left.mem_data;
		}
		else {
			// write the alu result into register Rdest
			core->registers[left.Rdest] = left.aluresult;
		}
		core->registers[0] = 0; // wire back to zero
	}
}


void InstructionDecodeStage::Shift()
{
	left = core->ifs.right;
}


void ExecuteStage::Shift()
{
	left = core->ids.right;
}


void MemoryStage::Shift()
{
	left = core->exs.right;
}


void WriteBackStage::Shift()
{
	left = core->mys.right;
}


void InstructionDecodeStage::Resolve()
{
	// Check for a data stall. If found, back up the machine one cycle. -----
	if (core->ids.right.control()->mem_read &&
		(core->ids.right.Rdest == core->ifs.right.Rsrc1 ||
			core->ids.right.Rdest == core->ifs.right.Rsrc2)) {
		// Bubble the pipe!
		// nop the two stalled instructions, back up the pc by 2 instructions
		if (core->verbose) {
			printf("*** BUBBLE\n");
		}
		core->ifs.make_nop();
		core->PC -= 8;
	}
	// ----------------------------------------------------------------------
}


void ExecuteStage::Resolve()
{
	// If the previous instruction is attempting a READ of the same register the instruction
	// in this stage is supposed to WRITE, then here, update the previous stage's right latch
	// with the value coming out of the ALU. (also, not reading zero reg)
	if (core->exs.right.Rdest != 0 && core->exs.right.control()->register_write) {
		if (core->exs.right.Rdest == core->ids.right.Rsrc1) {
			core->ids.right.Rsrc1Val = core->exs.right.aluresult;
			if (core->verbose) {
				printf("*** FORWARDex1:  %08x going to ID/EX's Rsrc1Val \n",
					core->exs.right.aluresult);
			}
		}
		if (core->exs.right.Rdest == core->ids.right.Rsrc2) {
			core->ids.right.Rsrc2Val = core->exs.right.aluresult;
			if (core->verbose) {
				printf("*** FORWARDex2:  %08x going to ID/EX's Rsrc2Val \n",
					core->exs.right.aluresult);
			}
		}
	}
}


void MemoryStage::Resolve()
{
	// If the next to previous instruction (IDS) is attempting a READ of the same register the instruction
	// in this stage is supposed to WRITE, then here, update the next-to-previous stage's right latch
	// with the value coming out of the this stage. (also, the read is not on zero)
	if (core->mys.right.Rdest != 0 && core->mys.right.control()->register_write) {
		if (core->exs.right.Rdest != core->ids.right.Rsrc1 &&
			core->mys.right.Rdest == core->ids.right.Rsrc1) {
			core->ids.right.Rsrc1Val =
				core->mys.right.control()->mem_read ? core->mys.right.mem_data : core->mys.right.aluresult;
			if (core->verbose) {
				printf("*** FORWARDmem1: %08x going to ID/EX's Rsrc1Val \n",
					core->ids.right.Rsrc1Val);
			}
		}
		if (core->exs.right.Rdest != core->ids.right.Rsrc2 &&
			core->mys.right.Rdest == core->ids.right.Rsrc2) {
			core->ids.right.Rsrc2Val =
				core->mys.right.control()->mem_read ? core->mys.right.mem_data : core->mys.right.aluresult;
			if (core->verbose) {
				printf("*** FORWARDmem2: %08x going to ID/EX's Rsrc2Val \n",
					core->ids.right.Rsrc2Val);
			}
		}
	}
}
