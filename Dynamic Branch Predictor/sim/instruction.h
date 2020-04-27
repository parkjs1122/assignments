#include <vector>
#include <stdio.h>

#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

// Function pointer for "special" instructions
class cpu_core;
typedef void(*operation)(cpu_core *cpu);
void sysc_op(cpu_core *cpu);

// Information about each instruction in the cpu, including function pointer and a
// character description for debugging purposes.
typedef struct _instruction {
	char name[9];
	operation special_case; // if nonnull handled by some magical process.
	const bool register_write;

	const byte alu_operation;  // an integer which determines which operation the ALU should perform
							   // 0: do nothing, this operation does not require an ALU op (copy forward)
							   // 1: do a signed add of reg1 to param
							   // 2: do a signed subtract of param from reg1
	const byte alu_source;     // second arg source
							   // 0: source from register.
							   // 1: source from immediate.
							   // 2: source from sum of immidate and register (addrcalc)

	const bool branch;         // whether it is branch operation or not
							   // 1: branch
							   // 0: not branch
	const byte mem_write;
	const byte mem_read;
	const bool mem_to_register;
} instruction;

// Define Branch Target Buffer structrue
typedef struct _btb {
	uint32_t branch_instruction_addr; // 32-bit
	uint32_t branch_target_addr; // 32-bit
	uint32_t prediction:2; // 2-bit
} BTB;

// Declare Branch Traget Buffer Table with vector
static std::vector<BTB> branch_target_buffer;

// Save options read from user and BTB size
static int BTB_ENTRIES = 0, PREDICTOR_TYPE = 0, BTB_COUNT = 0;

// Function of finding index of buffer with branch instruction address
static int btb_exist(uint32_t inst_addr) {
	for (int i = 0; i < branch_target_buffer.size(); i++) {
		if (branch_target_buffer[i].branch_instruction_addr == inst_addr) {
			return i;
		}
	}

	return -1;
}

// Function to decrease prediction bits
static void btb_delete(uint32_t inst_addr) {
	int btb_idx = btb_exist(inst_addr);

	if (btb_idx > -1) {
		if (branch_target_buffer[btb_idx].prediction > 0) { // In case prediction bits is greater than 1
			branch_target_buffer[btb_idx].prediction -= 1;
		}
	}
}

// Function to add new buffer or increase prediction bits
static void btb_add(uint32_t inst_addr, uint32_t target_addr) {
	int btb_idx = btb_exist(inst_addr);

	if (btb_idx > -1) {
		if (PREDICTOR_TYPE == 1) { // In case of 1-bit predictor
			if (branch_target_buffer[btb_idx].prediction < 1) {
				branch_target_buffer[btb_idx].prediction += 1;
			}
		}else if (PREDICTOR_TYPE == 2) { // In case of 2-bit predictor
			if (branch_target_buffer[btb_idx].prediction < 3) {
				branch_target_buffer[btb_idx].prediction += 1;
			}
		}
	}
	else { // Adding new buffer
		if (BTB_COUNT < BTB_ENTRIES) {
			BTB buffer;

			buffer.branch_instruction_addr = inst_addr;
			buffer.branch_target_addr = target_addr;
			if (PREDICTOR_TYPE == 1) buffer.prediction = 1;
			else if (PREDICTOR_TYPE == 2) buffer.prediction = 2;

			branch_target_buffer.push_back(buffer);

			BTB_COUNT += 1;
		}
	}
}

// opcode is the instruction's position in the array.
const instruction instructions[11] =
{ { "    nop", NULL,     false, 0, 0, false, 0, 0, false }     // 00
,{ "   addi", NULL,     true,  1, 1, false, 0, 0, false }     // 01
,{ "   beqz", NULL,     false, 0, 0, true,  0, 0, false }     // 02
,{ "    bge", NULL,     false, 0, 0, true,  0, 0, false }     // 03
,{ "    bne", NULL,     false, 0, 0, true,  0, 0, false }     // 04
,{ "     la", NULL,     true,  0, 1, false, 0, 0, false }     // 05
,{ "     lb", NULL,     true,  0, 2, false, 0, 1, true }     // 06
,{ "     li", NULL,     true,  0, 1, false, 0, 0, false }     // 07
,{ "   subi", NULL,     true,  2, 1, false, 0, 0, false }     // 08
,{ "    add", NULL,     true,  1, 0, false, 0, 0, false }     // 09
,{ "syscall", &sysc_op, false, 0, 0, false, 0, 0, false } };  // 0a

#endif /* _INSTRUCTION_H_ */
