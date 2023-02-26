	.syntax unified
	.cpu cortex-m7
	.thumb

	.section .text.pixmap8x16
	.weak pixmap8x16
	.type pixmap8x16, %function

// draw stippled monochrome pixel map to fb
// map comprised of 16 pixel rows and 8 pixel columns
// r0: color
// r1: output ptr
// r2: pixmap ptr
// clobbers: r3
//
// control symbols:
// fb_w (numerical): fb width
// fb_h (numerical): fb height

	.balign 32
pixmap8x16:
	stmdb	sp!,{r4-r5}
	movs	r3,#0
.Lbyte:
	ldrd	r4,r5,[r2],#8
	rev	r4,r4
	rev	r5,r5
.Lbit0:
	lsls	r4,r4,#1
	bcc	.Lpixel_done0
	strh	r0,[r1,r3]
.Lpixel_done0:
	adds	r3,r3,#2
	tst	r3,#15
	bne	.Lbit0
	adds	r1,r1,#(fb_w * 2) - 16
.Lbit1:
	lsls	r4,r4,#1
	bcc	.Lpixel_done1
	strh	r0,[r1,r3]
.Lpixel_done1:
	adds	r3,r3,#2
	tst	r3,#15
	bne	.Lbit1
	adds	r1,r1,#(fb_w * 2) - 16
.Lbit2:
	lsls	r4,r4,#1
	bcc	.Lpixel_done2
	strh	r0,[r1,r3]
.Lpixel_done2:
	adds	r3,r3,#2
	tst	r3,#15
	bne	.Lbit2
	adds	r1,r1,#(fb_w * 2) - 16
.Lbit3:
	lsls	r4,r4,#1
	bcc	.Lpixel_done3
	strh	r0,[r1,r3]
.Lpixel_done3:
	adds	r3,r3,#2
	tst	r3,#15
	bne	.Lbit3
	adds	r1,r1,#(fb_w * 2) - 16

	tst	r3,#256
	bne	.Ldone

	tst	r3,#127
	beq	.Lbyte

	movs	r4,r5
	b	.Lbit0
.Ldone:
	ldmia	sp!,{r4-r5}
	bx	lr

	.size pixmap8x16, . - pixmap8x16

	.section .text.pixmap8x8
	.weak pixmap8x8
	.type pixmap8x8, %function

// draw stippled monochrome pixel map to fb
// map comprised of 8 pixel rows and 8 pixel columns
// r0: color
// r1: output ptr
// r2: pixmap ptr
// clobbers: r3
//
// control symbols:
// fb_w (numerical): fb width
// fb_h (numerical): fb height

	.balign 32
pixmap8x8:
	stmdb	sp!,{r4-r5}
	movs	r3,#0
	ldrd	r4,r5,[r2],#8
	rev	r4,r4
	rev	r5,r5
.LLbit0:
	lsls	r4,r4,#1
	bcc	.LLpixel_done0
	strh	r0,[r1,r3]
.LLpixel_done0:
	adds	r3,r3,#2
	tst	r3,#15
	bne	.LLbit0
	adds	r1,r1,#(fb_w * 2) - 16
.LLbit1:
	lsls	r4,r4,#1
	bcc	.LLpixel_done1
	strh	r0,[r1,r3]
.LLpixel_done1:
	adds	r3,r3,#2
	tst	r3,#15
	bne	.LLbit1
	adds	r1,r1,#(fb_w * 2) - 16
.LLbit2:
	lsls	r4,r4,#1
	bcc	.LLpixel_done2
	strh	r0,[r1,r3]
.LLpixel_done2:
	adds	r3,r3,#2
	tst	r3,#15
	bne	.LLbit2
	adds	r1,r1,#(fb_w * 2) - 16
.LLbit3:
	lsls	r4,r4,#1
	bcc	.LLpixel_done3
	strh	r0,[r1,r3]
.LLpixel_done3:
	adds	r3,r3,#2
	tst	r3,#15
	bne	.LLbit3
	adds	r1,r1,#(fb_w * 2) - 16

	tst	r3,#128
	bne	.LLdone

	movs	r4,r5
	b	.LLbit0
.LLdone:
	ldmia	sp!,{r4-r5}
	bx	lr

	.size pixmap8x8, . - pixmap8x8
