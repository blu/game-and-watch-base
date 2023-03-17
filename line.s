	.syntax unified
	.cpu cortex-m7
	.thumb

	.section .text.line
	.weak line
	.type line, %function

// draw a line in a statically-sized fb; last pixel omitted
// r0.w: x0 (r0[31:16] replicate sign)
// r1.w: y0 (r1[31:16] replicate sign)
// r2.w: x1 (r2[31:16] replicate sign)
// r3.w: y1 (r3[31:16] replicate sign)
// r8: color (splatted to pixel word)
// r12: fb ptr
// clobbers: r4-r7, r9
//
// control symbols:
// fb_w (numerical): fb width
// fb_h (numerical): fb height

	.balign 32
line:
	// compute x0,y0 addr in fb
	movs	r4,#(fb_w * 2)
	smulbb	r4,r4,r1
	adds	r12,r12,r4
	adds	r12,r12,r0,lsl #1

	movs	r6,#1
	subs	r4,r2,r0 // dx
	bge	.Ldx_done
	negs	r4,r4
	negs	r6,r6
.Ldx_done:
	movs	r7,#1
	movs	r9,#(fb_w * 2)
	subs	r5,r3,r1 // dy
	bge	.Ldy_done
	negs	r5,r5
	negs	r7,r7
	negs	r9,r9
.Ldy_done:
	cmp	r5,r4
	bge	.Lhigh_slope
	// low slope: iterate along x
	lsls	r5,r5,#1 // 2 dy
	movs	r3,r5
	subs	r3,r3,r4 // 2 dy - dx
	lsls	r4,r4,#1 // 2 dx
.Lloop_x:
	strh	r8,[r12]
	adds	r12,r12,r6,lsl #1
	adds	r0,r0,r6
	tst	r3,r3
	ble	.Lx_done
	adds	r12,r12,r9
	subs	r3,r3,r4
.Lx_done:
	adds	r3,r3,r5
	cmp	r0,r2
	bne	.Lloop_x
	bx	lr
.Lhigh_slope: // iterate along y
	lsls	r4,r4,#1 // 2 dx
	movs	r2,r4
	subs	r2,r2,r5 // 2 dx - dy
	lsls	r5,r5,#1 // 2 dy
	bne	.Lloop_y
	bx	lr
.Lloop_y:
	strh	r8,[r12]
	adds	r12,r12,r9
	adds	r1,r1,r7
	tst	r2,r2
	ble	.Ly_done
	adds	r12,r12,r6,lsl #1
	subs	r2,r2,r5
.Ly_done:
	adds	r2,r2,r4
	cmp	r1,r3
	bne	.Lloop_y
	bx	lr

	.size line, . - line

	.section .text.line_clip
	.weak line_clip
	.type line_clip, %function

// draw a line in a statically-sized fb, clipping to fb bounds; last pixel omitted
// r0.w: x0 (r0[31:16] replicate sign)
// r1.w: y0 (r1[31:16] replicate sign)
// r2.w: x1 (r2[31:16] replicate sign)
// r3.w: y1 (r3[31:16] replicate sign)
// r8: color (splatted to pixel word)
// r12: fb ptr
// clobbers: r4-r7, r9
//
// control symbols:
// fb_w (numerical): fb width
// fb_h (numerical): fb height

	.balign 32
line_clip:
	// compute x0,y0 addr in fb
	movs	r4,#(fb_w * 2)
	smulbb	r4,r4,r1
	adds	r12,r12,r4
	adds	r12,r12,r0,lsl #1

	movs	r6,#1
	subs	r4,r2,r0 // dx
	bge	.LLdx_done
	negs	r4,r4
	negs	r6,r6
.LLdx_done:
	movs	r7,#1
	movs	r9,#(fb_w * 2)
	subs	r5,r3,r1 // dy
	bge	.LLdy_done
	negs	r5,r5
	negs	r7,r7
	negs	r9,r9
.LLdy_done:
	cmp	r5,r4
	bge	.LLhigh_slope
	// low slope: iterate along x
	lsls	r5,r5,#1 // 2 dy
	movs	r3,r5
	subs	r3,r3,r4 // 2 dy - dx
	lsls	r4,r4,#1 // 2 dx
.LLloop_x:
	cmp	r0,#fb_w
	bcs	.LLadvance_x
	cmp	r1,#fb_h
	bcs	.LLadvance_x
	strh	r8,[r12]
.LLadvance_x:
	adds	r12,r12,r6,lsl #1
	adds	r0,r0,r6
	tst	r3,r3
	ble	.LLx_done
	adds	r12,r12,r9
	subs	r3,r3,r4
	adds	r1,r1,r7
.LLx_done:
	adds	r3,r3,r5
	cmp	r0,r2
	bne	.LLloop_x
	bx	lr
.LLhigh_slope:
	lsls	r4,r4,#1 // 2 dx
	movs	r2,r4
	subs	r2,r2,r5 // 2 dx - dy
	lsls	r5,r5,#1 // 2 dy
	bne	.LLloop_y
	bx	lr
.LLloop_y:
	cmp	r0,#fb_w
	bcs	.LLadvance_y
	cmp	r1,#fb_h
	bcs	.LLadvance_y
	strh	r8,[r12]
.LLadvance_y:
	adds	r12,r12,r9
	adds	r1,r1,r7
	tst	r2,r2
	ble	.LLy_done
	adds	r12,r12,r6,lsl #1
	subs	r2,r2,r5
	adds	r0,r0,r6
.LLy_done:
	adds	r2,r2,r4
	cmp	r1,r3
	bne	.LLloop_y
	bx	lr

	.size line_clip, . - line_clip

	.section .text.line_full
	.weak line_full
	.type line_full, %function

// draw a line in a statically-sized fb; last pixel drawn
// r0.w: x0 (r0[31:16] replicate sign)
// r1.w: y0 (r1[31:16] replicate sign)
// r2.w: x1 (r2[31:16] replicate sign)
// r3.w: y1 (r3[31:16] replicate sign)
// r8: color (splatted to pixel word)
// r12: fb ptr
// clobbers: r4-r7, r9
//
// control symbols:
// fb_w (numerical): fb width
// fb_h (numerical): fb height

	.balign 32
line_full:
	// compute x0,y0 addr in fb
	movs	r4,#(fb_w * 2)
	smulbb	r4,r4,r1
	adds	r12,r12,r4
	adds	r12,r12,r0,lsl #1

	movs	r6,#1
	subs	r4,r2,r0 // dx
	bge	.LLLdx_done
	negs	r4,r4
	negs	r6,r6
.LLLdx_done:
	movs	r7,#1
	movs	r9,#(fb_w * 2)
	subs	r5,r3,r1 // dy
	bge	.LLLdy_done
	negs	r5,r5
	negs	r7,r7
	negs	r9,r9
.LLLdy_done:
	cmp	r5,r4
	bge	.LLLhigh_slope
	// low slope: iterate along x
	lsls	r5,r5,#1 // 2 dy
	movs	r3,r5
	subs	r3,r3,r4 // 2 dy - dx
	lsls	r4,r4,#1 // 2 dx
.LLLloop_x:
	strh	r8,[r12]
	adds	r12,r12,r6,lsl #1
	adds	r0,r0,r6
	tst	r3,r3
	ble	.LLLx_done
	adds	r12,r12,r9
	subs	r3,r3,r4
.LLLx_done:
	adds	r3,r3,r5
	cmp	r0,r2
	bne	.LLLloop_x
	strh	r8,[r12]
	bx	lr
.LLLhigh_slope: // iterate along y
	lsls	r4,r4,#1 // 2 dx
	movs	r2,r4
	subs	r2,r2,r5 // 2 dx - dy
	lsls	r5,r5,#1 // 2 dy
	bne	.LLLloop_y
	strh	r8,[r12]
	bx	lr
.LLLloop_y:
	strh	r8,[r12]
	adds	r12,r12,r9
	adds	r1,r1,r7
	tst	r2,r2
	ble	.LLLy_done
	adds	r12,r12,r6,lsl #1
	subs	r2,r2,r5
.LLLy_done:
	adds	r2,r2,r4
	cmp	r1,r3
	bne	.LLLloop_y
	strh	r8,[r12]
	bx	lr

	.size line_full, . - line_full
