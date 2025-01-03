﻿#pragma once

#include "sim8086.h"
#include "sim8086_types.h"


b32
IsPrintable(instruction Instruction);

void
PrintInstruction(instruction Instruction, FILE *Dest);

void
PrintRegistersState(FILE *Dest); 

const char*
GetFlagRegisterName(u8 Flag);

void
PrintFlagsRegister();