	.syntax unified
	.cpu cortex-m7
	.thumb

/* vertex coord component, aka R */
.equ	R_size, 2 /* short */

/* struct R2 */
.equ	R2_x, R_size * 0 /* short */
.equ	R2_y, R_size * 1 /* short */
.equ	R2_size, R_size * 2

/* struct pb (parallelogram basis) */
.equ	pb_e01,  R2_size * 0 /* R2 */
.equ	pb_e02,  R2_size * 1 /* R2 */
.equ	pb_area, R2_size * 2 /* word */
.equ	pb_size, R2_size * 2 + 4

	.section .text.init_pb
	.weak init_pb
	.type init_pb, %function

// compute parallelogram basis from a tri:
//   e01 = p1 - p0
//   e02 = p2 - p0
//   area = e01.x * e02.y - e02.x * e01.y
// r0.w: p0.x
// r1.w: p0.y
// r2.w: p1.x
// r3.w: p1.y
// r4.w: p2.x
// r5.w: p2.y
// r12: parallelogram basis ptr
// returns: cc: Z: zero area
//              N,V: polarity of area
	.balign 32
init_pb:
	subs	r2,r2,r0 // e01.x = p1.x - p0.x
	subs	r3,r3,r1 // e01.y = p1.y - p0.y

	subs	r4,r4,r0 // e02.x = p2.x - p0.x
	subs	r5,r5,r1 // e02.y = p2.y - p0.y

	strh	r2,[r12,#pb_e01+R2_x]
	strh	r3,[r12,#pb_e01+R2_y]

	strh	r4,[r12,#pb_e02+R2_x]
	strh	r5,[r12,#pb_e02+R2_y]

	// area = e01.x * e02.y - e02.x * e01.y
	smulbb	r2,r2,r5
	smulbb	r3,r3,r4
	subs	r2,r2,r3

	str	r2,[r12,#pb_area]
	bx	lr

	.size init_pb, . - init_pb

	.section .text.get_coord
	.weak get_coord
	.type get_coord, %function

// get barycentric coords of the given point in the given parallelogram basis;
// resulting coords are before normalization!
// r0.w: pt.x
// r1.w: pt.y
// r2.w: p0.x
// r3.w: p0.y
// r4: pb.e01.y, pb.e01.x ([31:16], [15:0])
// r5: pb.e02.y, pb.e02.x ([31:16], [15:0])
// returns: r0: s coord before normalization
//          r1: t coord before normalization
	.balign 32
get_coord:
	stmdb	sp!,{r2-r3}
	// dx = pt.x - p0.x
	// dy = pt.y - p0.y
	subs	r0,r0,r2
	subs	r1,r1,r3
	// s = dx * pb.e02.y - dy * pb.e02.x
	// t = dy * pb.e01.x - dx * pb.e01.y
	movs	r2,r0
	movs	r3,r1

	smulbt	r0,r0,r5 // dx * e02.y
	smulbb	r1,r1,r4 // dy * e01.x
	smulbt	r2,r2,r4 // dx * e01.y
	smulbb	r3,r3,r5 // dy * e02.x

	subs	r1,r1,r2
	subs	r0,r0,r3

	ldmia	sp!,{r2-r3}
	bx	lr

	.size get_coord, . - get_coord
