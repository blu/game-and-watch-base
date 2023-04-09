	.syntax unified
	.cpu cortex-m7
	.thumb

	.section .text.mul_vec3_mat
	.weak mul_vec3_mat
	.type mul_vec3_mat, %function

// multiply a 3d vector by a 3d row-major matrix:
// | v0, v1, v2 | * | x0, y0, z0 |
//                  | x1, y1, z1 |
//                  | x2, y2, z2 |
// input vector elements are int16, matrix elements are in some
// 32b fx format, e.g. fx16.16; product elements are in the same
// format, e.g. if matrix elements are fx16.16, so are results
// r0.w: v0 (bits [31:16] replicate sign)
// r1.w: v1 (bits [31:16] replicate sign)
// r2.w: v2 (bits [31:16] replicate sign)
// r3: mat_0.x
// r4: mat_0.y
// r5: mat_0.z
// r6: mat_1.x
// r7: mat_1.y
// r8: mat_1.z
// r9: mat_2.x
// r10:mat_2.y
// r11:mat_2.z
// returns:
// r3: rv0
// r4: rv1
// r5: rv2
	.balign 32
mul_vec3_mat:
	muls	r3,r3,r0
	muls	r4,r4,r0
	muls	r5,r5,r0

	mla	r3,r6,r1,r3
	mla	r4,r7,r1,r4
	mla	r5,r8,r1,r5

	mla	r3,r9, r2,r3
	mla	r4,r10,r2,r4
	mla	r5,r11,r2,r5
	bx	lr

	.size mul_vec3_mat, . - mul_vec3_mat
