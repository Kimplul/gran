.global _start
_start:
/* sum from 0 to 1000000 */

li a0, 0	/* index */
li a1, 1000000	/* top */
li a2, 0	/* sum */

top:
beq a0, a1, done
add a2, a2, a0
addi a0, a0, 1
j top

done:
ebreak
