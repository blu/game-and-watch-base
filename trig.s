	.syntax unified
	.cpu cortex-m7
	.thumb

	.section .text.sin16
	.weak sin16
	.type sin16, %function

// get sine
// r0: angle ticks -- [0, 2pi) -> [0, 256)
// returns: r0: sine as fx16.16
// clobbers: r1, r12

	.balign 32
sin16:
	movs	r1,#1
	ands	r0,r0,#0xff
	cmp	r0,#0x80
	bcc	.Lsign_done
	negs	r1,r1
	subs	r0,r0,#0x80
.Lsign_done:
	cmp	r0,#0x40
	bcc	.Lfetch
	bne	.Lnot_maximum
	movs	r0,#0x10000
	b	.Lcorr_sign
.Lnot_maximum:
	rsbs	r0,r0,#0x80
.Lfetch:
	adr	r12,.LsinLUT16_64
	ldrh	r0,[r12,r0,lsl #1]
.Lcorr_sign:
	muls	r0,r0,r1
	bx	lr
	.balign 32
.LsinLUT16_64:
	.short 0x0000, 0x0648, 0x0C90, 0x12D5, 0x1918, 0x1F56, 0x2590, 0x2BC4
	.short 0x31F1, 0x3817, 0x3E34, 0x4447, 0x4A50, 0x504D, 0x563E, 0x5C22
	.short 0x61F8, 0x67BE, 0x6D74, 0x731A, 0x78AD, 0x7E2F, 0x839C, 0x88F6
	.short 0x8E3A, 0x9368, 0x9880, 0x9D80, 0xA268, 0xA736, 0xABEB, 0xB086
	.short 0xB505, 0xB968, 0xBDAF, 0xC1D8, 0xC5E4, 0xC9D1, 0xCD9F, 0xD14D
	.short 0xD4DB, 0xD848, 0xDB94, 0xDEBE, 0xE1C6, 0xE4AA, 0xE76C, 0xEA0A
	.short 0xEC83, 0xEED9, 0xF109, 0xF314, 0xF4FA, 0xF6BA, 0xF854, 0xF9C8
	.short 0xFB15, 0xFC3B, 0xFD3B, 0xFE13, 0xFEC4, 0xFF4E, 0xFFB1, 0xFFEC

	.size sin16, . - sin16

	.section .text.cos16
	.weak cos16
	.type cos16, %function

// get cosine
// r0: angle ticks -- [0, 2pi) -> [0, 256)
// returns; r0: cosine as fx16.16
// clobbers: r1, r12

cos16:
	adds	r0,r0,#0x40
	b	sin16

	.size cos16, . - cos16

	.section .text.sin15
	.weak sin15
	.type sin15, %function

// get sine
// r0: angle ticks -- [0, 2pi) -> [0, 256)
// returns; r0: sine as fx16.15 (r0[31] replicates sign)
// clobbers: r12

sin15:
	subs	r0,r0,#0x40
	b	cos15

	.size sin15, . - sin15

	.section .text.cos15
	.weak cos15
	.type cos15, %function

// get cosine
// r0: angle ticks -- [0, 2pi) -> [0, 256)
// returns: r0: cosine as fx16.15 (r0[31] replicates sign)
// clobbers: r12

	.balign 32
cos15:
	ands	r0,r0,#0xff
	bne	.Lnonzero
	movs	r0,#0x8000
	bx	lr
.Lnonzero:
	cmp	r0,#0x80
	bls	.LLfetch
	rsbs	r0,r0,#0x100
.LLfetch:
	adr	r12,.LcosLUT15_128 - 2
	ldrh	r0,[r12,r0,lsl #1]
	sxth	r0,r0
	bx	lr

	.balign 32
.LcosLUT15_128:
	.short 0x7FF6, 0x7FD9, 0x7FA7, 0x7F62, 0x7F0A, 0x7E9D, 0x7E1E, 0x7D8A
	.short 0x7CE4, 0x7C2A, 0x7B5D, 0x7A7D, 0x798A, 0x7885, 0x776C, 0x7642
	.short 0x7505, 0x73B6, 0x7255, 0x70E3, 0x6F5F, 0x6DCA, 0x6C24, 0x6A6E
	.short 0x68A7, 0x66D0, 0x64E9, 0x62F2, 0x60EC, 0x5ED7, 0x5CB4, 0x5A82
	.short 0x5843, 0x55F6, 0x539B, 0x5134, 0x4EC0, 0x4C40, 0x49B4, 0x471D
	.short 0x447B, 0x41CE, 0x3F17, 0x3C57, 0x398D, 0x36BA, 0x33DF, 0x30FC
	.short 0x2E11, 0x2B1F, 0x2827, 0x2528, 0x2224, 0x1F1A, 0x1C0C, 0x18F9
	.short 0x15E2, 0x12C8, 0x0FAB, 0x0C8C, 0x096B, 0x0648, 0x0324, 0x0000
	.short -0x0324, -0x0648, -0x096B, -0x0C8C, -0x0FAB, -0x12C8, -0x15E2, -0x18F9
	.short -0x1C0C, -0x1F1A, -0x2224, -0x2528, -0x2827, -0x2B1F, -0x2E11, -0x30FC
	.short -0x33DF, -0x36BA, -0x398D, -0x3C57, -0x3F17, -0x41CE, -0x447B, -0x471D
	.short -0x49B4, -0x4C40, -0x4EC0, -0x5134, -0x539B, -0x55F6, -0x5843, -0x5A82
	.short -0x5CB4, -0x5ED7, -0x60EC, -0x62F2, -0x64E9, -0x66D0, -0x68A7, -0x6A6E
	.short -0x6C24, -0x6DCA, -0x6F5F, -0x70E3, -0x7255, -0x73B6, -0x7505, -0x7642
	.short -0x776C, -0x7885, -0x798A, -0x7A7D, -0x7B5D, -0x7C2A, -0x7CE4, -0x7D8A
	.short -0x7E1E, -0x7E9D, -0x7F0A, -0x7F62, -0x7FA7, -0x7FD9, -0x7FF6, 0x8000

	.size cos15, . - cos15

	.section .text.sin
	.weak sin
	.type sin, %function

// get sine
// r0: angle ticks -- [0, 2pi) -> [0, 256)
// r12: lut ptr - 2
// returns; r0: sine as fx16.15 (r0[31] replicates sign)

sin:
	subs	r0,r0,#0x40
	b	cos

	.size sin, . - sin

	.section .text.cos
	.weak cos
	.type cos, %function

// get cosine
// r0: angle ticks -- [0, 2pi) -> [0, 256)
// r12: lut ptr - 2
// returns: r0: cosine as fx16.15 (r0[31] replicates sign)

	.balign 32
cos:
	ands	r0,r0,#0xff
	bne	.LLnonzero
	movs	r0,#0x8000
	bx	lr
.LLnonzero:
	cmp	r0,#0x80
	bls	.LLLfetch
	rsbs	r0,r0,#0x100
.LLLfetch:
	ldrh	r0,[r12,r0,lsl #1]
	sxth	r0,r0
	bx	lr

	.size cos, . - cos

	.section .data.cosLUT15_128
	.weak cosLUT15_128
	.type cosLUT15_128, %object

	.balign 32
cosLUT15_128:
	.short 0x7FF6, 0x7FD9, 0x7FA7, 0x7F62, 0x7F0A, 0x7E9D, 0x7E1E, 0x7D8A
	.short 0x7CE4, 0x7C2A, 0x7B5D, 0x7A7D, 0x798A, 0x7885, 0x776C, 0x7642
	.short 0x7505, 0x73B6, 0x7255, 0x70E3, 0x6F5F, 0x6DCA, 0x6C24, 0x6A6E
	.short 0x68A7, 0x66D0, 0x64E9, 0x62F2, 0x60EC, 0x5ED7, 0x5CB4, 0x5A82
	.short 0x5843, 0x55F6, 0x539B, 0x5134, 0x4EC0, 0x4C40, 0x49B4, 0x471D
	.short 0x447B, 0x41CE, 0x3F17, 0x3C57, 0x398D, 0x36BA, 0x33DF, 0x30FC
	.short 0x2E11, 0x2B1F, 0x2827, 0x2528, 0x2224, 0x1F1A, 0x1C0C, 0x18F9
	.short 0x15E2, 0x12C8, 0x0FAB, 0x0C8C, 0x096B, 0x0648, 0x0324, 0x0000
	.short -0x0324, -0x0648, -0x096B, -0x0C8C, -0x0FAB, -0x12C8, -0x15E2, -0x18F9
	.short -0x1C0C, -0x1F1A, -0x2224, -0x2528, -0x2827, -0x2B1F, -0x2E11, -0x30FC
	.short -0x33DF, -0x36BA, -0x398D, -0x3C57, -0x3F17, -0x41CE, -0x447B, -0x471D
	.short -0x49B4, -0x4C40, -0x4EC0, -0x5134, -0x539B, -0x55F6, -0x5843, -0x5A82
	.short -0x5CB4, -0x5ED7, -0x60EC, -0x62F2, -0x64E9, -0x66D0, -0x68A7, -0x6A6E
	.short -0x6C24, -0x6DCA, -0x6F5F, -0x70E3, -0x7255, -0x73B6, -0x7505, -0x7642
	.short -0x776C, -0x7885, -0x798A, -0x7A7D, -0x7B5D, -0x7C2A, -0x7CE4, -0x7D8A
	.short -0x7E1E, -0x7E9D, -0x7F0A, -0x7F62, -0x7FA7, -0x7FD9, -0x7FF6, 0x8000

	.size cosLUT15_128, . - cosLUT15_128
