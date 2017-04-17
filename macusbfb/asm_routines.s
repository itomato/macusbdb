	.syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.section ".text"
	.thumb

	.global	_start
	.type   _start, %function    
_start:

/* fill .bss with 0 using byte stores */
	ldr     r0, =_bss
	ldr     r1, =_ebss
        mov     r2, #0
.L0:
	strb	r2, [r0], #1
	cmp     r0, r1
  	bcc.n   .L0

/* copy initialized .data from rom to ram */
	ldr     r0, =_data
	ldr     r1, =_edata
	ldr     r2, =_ldata
.L1:
	ldrb	r3, [r2], #1
	strb	r3, [r0], #1
	cmp     r0, r1
	bcc.n   .L1

/* branch to main */
	ldr     r0, =main+1
	bx      r0 
	.size	_start, .-_start


	.section ".CRP._0x02FC","a",%progbits
	.globl  CRP_Key
CRP_Key:
	.word     0xFFFFFFFF
	
	.end
