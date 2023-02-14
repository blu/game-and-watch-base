	.syntax unified
	.cpu cortex-m7
	.thumb

	.section .text.line
	.weak line
	.type line, %function
/*
  control symbols:
  fb_w (numerical): fb width
  fb_h (numerical): fb height
*/
/*
  draw a line in a statically-sized fb; last pixel omitted
  r0: x0
  r1: y0
  r2: x1
  r3: y1
  r8: color (splatted to pixel word)
  r12: fb ptr
  clobbers: r4-r7, r9
*/
	.balign 32
line:
/* compute x0,y0 addr in fb */
	movs	r4,#(fb_w * 2)
	smulbb	r4,r4,r1
	adds	r12,r12,r4
	adds	r12,r12,r0,lsl #1

	movs	r6,#1
	subs	r4,r2,r0 /* dx */
	bge	dx_done
	negs	r4,r4
	negs	r6,r6
dx_done:
	movs	r7,#1
	movs	r9,#(fb_w * 2)
	subs	r5,r3,r1 /* dy */
	bge	dy_done
	negs	r5,r5
	negs	r7,r7
	negs	r9,r9
dy_done:
	cmp	r5,r4
	bge	high_slope
/* low slope: iterate along x */
	lsls	r5,r5,#1 /* 2 dy */
	movs	r3,r5
	subs	r3,r3,r4 /* 2 dy - dx */
	lsls	r4,r4,#1 /* 2 dx */
loop_x:
	strh	r8,[r12]
advance_x:
	adds	r12,r12,r6,lsl #1
	adds	r0,r0,r6
	tst	r3,r3
	ble	x_done
	adds	r12,r12,r9
	subs	r3,r3,r4
x_done:
	adds	r3,r3,r5
	cmp	r0,r2
	bne	loop_x
	bx	lr
high_slope:
	lsls	r4,r4,#1 /* 2 dx */
	movs	r2,r4
	subs	r2,r2,r5 /* 2 dx - dy */
	lsls	r5,r5,#1 /* 2 dy */
	bne	loop_y
	bx	lr
loop_y:
	strh	r8,[r12]
advance_y:
	adds	r12,r12,r9
	adds	r1,r1,r7
	tst	r2,r2
	ble	y_done
	adds	r12,r12,r6,lsl #1
	subs	r2,r2,r5
y_done:
	adds	r2,r2,r4
	cmp	r1,r3
	bne	loop_y
	bx	lr

	.size line, . - line
