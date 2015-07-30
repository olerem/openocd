#include <mips/regdef.h>
#include <mips/m32c0.h>

		.text
		.globl	__start
		.ent __start

__start:
		lui sp, 0xA000
		ori sp, sp, __stack
		sw  zero, 0(sp)
		move a0, zero
		move a1, zero

		lui gp, 0xa001
		ori gp, gp, _gp

		jal main
		nop

		.end __start

		.data
		.globl __stack
stack_end:
		.space  4000,0		
__stack:
		.long 		1
