#pragma once

#include "sim8086.h"
#include "sim8086_types.h"

b32 IsPrintable(instruction Instruction);
void PrintInstruction(instruction Instruction, FILE *Dest);