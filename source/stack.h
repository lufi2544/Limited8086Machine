/* date = November 2nd 2024 1:03 pm */

/*
  This is the file where all the stack related classes are.

Stack: The stack tries to solve a problem in software, which is the call of routines and the usage of the registers 
along them but with keeping the control from one routine to another for them to not override the registers values.
The stack alsom implies that local variables can be used and are cleared at the end of the routine.

So if we have procedure 1 and call procedure 2, once we are done with the 2, we return to 1 and wish to encounter all the resiters
as they were.

STACK FEATURES:

- 1. Routines calling with independent registers states.
- 2. Local Variables with a routine life span.
- 3. Parameters passing to the routine.

 NOTE: The registers used for parameters has to be passed in and pushed to the stack before the procedure call,
once the procedure has finished, we have to restore the space for the local variables first and then restore the
space for the used registers.

**IMPORTANT** If we passed any parameters, we have to restore the space for them in the satck, if we didn't, they would
be tanggling in the stack for ever. We can use in the 8086 the RET 4(being the number of words reserver for the params).

Stack memory is allocated at the begining of the program.( In the decoder I am going to allocate it manually, but in the 8086
 we had to do this manually)

SP - Register that stores the value of the current stack ptr. 
BP - Base pointer of the Procedure, used for refereing to local variables.
IP - Program Instruction pointer, used for jumping between procedures.


For storing data in the Stack Segmenet Addresses, I am going to store the data from HighBits to Low, as the Stack grows downwards

E.g:
-> SP - 255, if we push AX(10), the 10 value bits will be in the 253 and the 0 bit value will be in the 254. 
@see Push and Pop

   
  

*/
#ifndef STACK_H
#define STACK_H

#include "sim8086_types.h"
#include "sim8086.h"

struct memory;

struct stack
{
	memory* Memory = nullptr;
	
	void Push(register_index Register);
    u32 Pop(register_index Register, u8 Num = 0);
};



#endif //STACK_H
