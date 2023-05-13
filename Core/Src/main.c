/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <assert.h>
#include "main.h"
#include <core_cm7.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "buttons.h"
#include "flash.h"
#include "lcd.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

LTDC_HandleTypeDef hltdc;

OSPI_HandleTypeDef hospi1;

SAI_HandleTypeDef hsai_BlockA1;
DMA_HandleTypeDef hdma_sai1_a;

SPI_HandleTypeDef hspi2;

uint16_t framebuffer[2][320 * 240]  __attribute__((section (".lcd")));

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_LTDC_Init(void);
static void MX_SPI2_Init(void);
static void MX_OCTOSPI1_Init(void);
static void MX_SAI1_Init(void);
static void MX_NVIC_Init(void);

extern int32_t sin15(uint16_t ticks);
extern int32_t cos15(uint16_t ticks);
extern int32_t sin16(uint16_t ticks);
extern int32_t cos16(uint16_t ticks);

extern void pixmap8x16(uint16_t color, void *out_fb, void *pixmap);
extern void pixmap8x8(uint16_t color, void *out_fb, void *pixmap);

/* Following are mere labels; use only by-address */
extern uint8_t fnt_wang_8x16;
extern uint8_t fnt_wang_8x8;
extern uint8_t mesh_obj_0;
extern uint8_t mesh_obj_1;
extern uint8_t mesh_obj_2;
extern uint8_t mesh_obj_3;
extern uint8_t mesh_obj_4;
extern uint8_t mesh_obj_5;
extern uint8_t mesh_obj_6;

void *mesh_obj = &mesh_obj_0;
void *mseq[] = {
	&mesh_obj_0,
	&mesh_obj_1,
	&mesh_obj_2,
	&mesh_obj_3,
	&mesh_obj_4,
	&mesh_obj_5,
	&mesh_obj_6
};

short tri_obj_0[2 * 3 * 4] __attribute__ ((used)) = {
	  0, -29,
	 25,  14,
	-25,  14,
};

static void deepsleep() __attribute__ ((noinline));
static void print_x32(uint16_t color, void *out_fb, uint32_t val) __attribute__ ((noinline));

#define ALT_NUM	18

uint32_t cycle[ALT_NUM];

typedef struct {
	int16_t pos[3];
} Vertex;

typedef uint16_t Index;

typedef struct {
	uint16_t count;
	Vertex arr[1];
} count_arr_t;

typedef struct {
	uint16_t count;
	Index idx[1][3];
} count_idx_t;

static void createIndexedPolarSphere(
	count_arr_t *count_arr,
	count_idx_t *count_idx,
	const int rows,
	const int cols,
	const float r) __attribute__ ((noinline));
/**
  * @brief Bend a polar sphere from a grid of the specified dimensions
  * @param count_arr: Counted vertex array to fill; must provide sufficient space for the specified mesh
  * @param count_idx: Counted index array to fill; must provide sufficient space for the specified mesh
  * @param rows: Rows of the mesh; minimum 3
  * @param cols: Columns of the mesh; minimum 4
  * @param r: Sphere radius
  * @retval None
  *
  * Example 4x5 grid:
  *
  *   0       1       2       3       4
  * 0     +       +       +       +
  *     /       /       /       /
  * 1 +       +       +       +       +
  *   |       |       |       |       |
  * 2 +       +       +       +       +
  *         /       /       /       /
  * 3     +       +       +       +
  */
static void createIndexedPolarSphere(
	count_arr_t *count_arr,
	count_idx_t *count_idx,
	const int rows,
	const int cols,
	const float r)
{
	assert(rows > 2);
	assert(cols > 3);

	const size_t num_verts __attribute__ ((unused)) = (rows - 2) * cols + 2 * (cols - 1);
	const size_t num_tris __attribute__ ((unused)) = ((rows - 3) * 2 + 2) * (cols - 1);

	assert(num_verts <= (uint16_t) -1U);
	assert(num_tris <= (uint16_t) -1U);

	Vertex *arr = count_arr->arr;
	Index (*idx)[3] = count_idx->idx;

	unsigned ai = 0;

	// north pole
	for (int j = 0; j < cols - 1; ++j) {
		assert(ai < num_verts);

		arr[ai].pos[0] = 0.f;
		arr[ai].pos[1] = 0.f;
		arr[ai].pos[2] = r;

		++ai;
	}

	// interior
	for (int i = 1; i < rows - 1; ++i)
		for (int j = 0; j < cols; ++j) {
			assert(ai < num_verts);

			const float azim = j * 2 * M_PI / (cols - 1);
			const float decl = M_PI_2 - i * M_PI / (rows - 1);
			const float sin_azim = sinf(azim);
			const float cos_azim = cosf(azim);
			const float sin_decl = sinf(decl);
			const float cos_decl = cosf(decl);

			arr[ai].pos[0] = r * cos_decl * cos_azim;
			arr[ai].pos[1] = r * cos_decl * sin_azim;
			arr[ai].pos[2] = r * sin_decl;

			++ai;
		}

	// south pole
	for (int j = 0; j < cols - 1; ++j) {
		assert(ai < num_verts);

		arr[ai].pos[0] = 0.f;
		arr[ai].pos[1] = 0.f;
		arr[ai].pos[2] = -r;

		++ai;
	}

	assert(ai == num_verts);
	count_arr->count = (uint16_t) ai;

	unsigned ii = 0;

	// north pole
	for (unsigned j = 0; j < cols - 1; ++j) {
		if (j & 1) continue; /* checker the shpere */

		assert(ii < num_tris);

		idx[ii][0] = (Index)(j);
		idx[ii][1] = (Index)(j + cols - 1);
		idx[ii][2] = (Index)(j + cols);
		++ii;
	}

	// interior
	for (int i = 1; i < rows - 2; ++i)
		for (int j = 0; j < cols - 1; ++j) {
			if ((j ^ i) & 1) continue; /* checker the shpere */

			assert(ii < num_tris);

			idx[ii][0] = (Index)(j + i * cols);
			idx[ii][1] = (Index)(j + i * cols - 1);
			idx[ii][2] = (Index)(j + (i + 1) * cols);
			++ii;

			assert(ii < num_tris);

			idx[ii][0] = (Index)(j + (i + 1) * cols - 1);
			idx[ii][1] = (Index)(j + (i + 1) * cols);
			idx[ii][2] = (Index)(j + i * cols - 1);
			++ii;
		}

	// south pole
	for (unsigned j = 0; j < cols - 1; ++j) {
		if ((j & 1) == (rows - 1 & 1)) continue; /* checker the shpere */

		assert(ii < num_tris);

		idx[ii][0] = (Index)(j + (rows - 2) * cols);
		idx[ii][1] = (Index)(j + (rows - 2) * cols - 1);
		idx[ii][2] = (Index)(j + (rows - 2) * cols + cols - 1);
		++ii;
	}

	assert(ii <= num_tris);
	count_idx->count = (uint16_t) ii;
}

/* Amiga Boing Ball (polar sphere 9x17) */
struct { uint16_t count; Vertex arr[151]; } sphere_obj;
struct { uint16_t count; Index idx[224][3]; } sphere_idx;

static void alt_rot2d_dots(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot2d_lines(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot2d_wire_triforce(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot2d_solid_triforce(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot2d_solid_tri(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot2d_wire_trilist_bc(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot2d_solid_trilist_bc(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));

static void alt_rot3d_wire_trilist(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot3d_wire_trilist_bc(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot3d_solid_trilist_bc(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_rot3d_solid_trilist_bc_zb(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));
static void alt_bounce_sphere(const uint16_t color, uint32_t ii, void *framebuffer) __attribute__ ((noinline));

static void alt_rot2d_dots(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse dots circling CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"
	/* plot CW-rotating dots */
		"movs	r3,#0xff\n\t"
	"2:\n\t"
		"adds	r0,r3,%[idx]\n\t"
		"bl	sin15\n\t"
		"movs	r1,#112\n\t"
		"muls	r0,r0,r1\n\t"
		"asrs	r0,r0,#15\n\t"
		"adcs	r0,r0,#120\n\t"

		"movs	r1,#640\n\t"
		"smulbb	r2,r1,r0\n\t"

		"adds	r0,r3,%[idx]\n\t"
		"bl	cos15\n\t"
		"movs	r1,#112\n\t"
		"muls	r0,r0,r1\n\t"
		"asrs	r0,r0,#15\n\t"
		"adcs	r1,r0,#160\n\t"

		"ldmia	sp,{%[color],%[fb]}\n\t"

		"adds	r2,r2,%[fb]\n\t"
		"mvns	%[color],%[color]\n\t"
		"str	%[color],[r2,r1,lsl #1]\n\t"
		"adds	r2,r2,#640\n\t"
		"str	%[color],[r2,r1,lsl #1]\n\t"

		"subs	r3,r3,#4\n\t"
		"bcs	2b\n\t"

		"ldmia	sp!,{%[color],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r14", "cc");
}

static void alt_rot2d_lines(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse lines rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"
	/* plot lines */
		"ldr	r8,[sp]\n\t"
		"mvns	r8,r8\n\t"
		"movs	r10,#0x7f\n\t"
	"2:\n\t"
		"rsbs	r0,r10,#0xff\n\t"
#if 0
		"adds	r0,r0,%[idx]\n\t"
#endif
		"bl	sin15\n\t"
		"movs	r1,#112\n\t"
		"muls	r0,r0,r1\n\t"
		"asrs	r0,r0,#15\n\t"
		"adcs	r2,r0,#120\n\t"
		"rsbs	r3,r0,#120\n\t"

		"rsbs	r0,r10,#0xff\n\t"
#if 1
		"adds	r0,r0,%[idx]\n\t"
#endif
		"bl	cos15\n\t"
		"movs	r1,#112\n\t"
		"muls	r0,r0,r1\n\t"
		"asrs	r0,r0,#15\n\t"
		"adcs	r4,r0,#160\n\t"

		"movs	r1,r2\n\t"
		"rsbs	r2,r0,#160\n\t"
		"movs	r0,r4\n\t"
		"ldr	%[fb],[sp,#4]\n\t"
		"bl	line_full\n\t"

		"subs	r10,r10,#8\n\t"
		"bcs	2b\n\t"

		"ldmia	sp!,{%[color],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_rot2d_wire_triforce(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse triangles rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* struct R2 */
	".equ	R2_x, 0\n\t" /* short */
	".equ	R2_y, 2\n\t" /* short */
	".equ	R2_size, 4\n\t"

	/* struct tri */
	".equ	tri_p0, R2_size * 0\n\t" /* R2 */
	".equ	tri_p1, R2_size * 1\n\t" /* R2 */
	".equ	tri_p2, R2_size * 2\n\t" /* R2 */
	".equ	tri_size, R2_size * 3\n\t"

	/* plot tris */
	/* transform obj -> scr */
		/* preload cos(idx * 1) */
		"lsls	r0,%[idx],#0\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx * 1) */
		"lsls	r0,%[idx],#0\n\t"
		"bl	sin15\n\t"

		"ldr	r8,=tri_obj_0\n\t"
		"adds	r9,r8,#tri_size\n\t"
		"movs	r10,r9\n\t"
	"20:\n\t"
		"ldrsh	r2,[r8],#2\n\t" /* v_in.x */
		"ldrsh	r3,[r8],#2\n\t" /* v_in.y */

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r4,r1,r2\n\t"
		"mul	r6,r0,r3\n\t"
		"subs	r4,r4,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r5,r0,r2\n\t"
		"mul	r6,r1,r3\n\t"
		"adds	r5,r5,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#120\n\t"

		"strh	r4,[r9],#2\n\t" /* v_out.x */
		"strh	r5,[r9],#2\n\t" /* v_out.y */

		"cmp	r8,r10\n\t"
		"bne	20b\n\t"

		/* preload cos(idx * 2) */
		"lsls	r0,%[idx],#1\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx * 2) */
		"lsls	r0,%[idx],#1\n\t"
		"bl	sin15\n\t"

		"ldr	r8,=tri_obj_0\n\t"
	"21:\n\t"
		"ldrsh	r2,[r8],#2\n\t" /* v_in.x */
		"ldrsh	r3,[r8],#2\n\t" /* v_in.y */

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r4,r1,r2\n\t"
		"mul	r6,r0,r3\n\t"
		"subs	r4,r4,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r4,r4,#14\n\t"
		"adcs	r4,r4,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r5,r0,r2\n\t"
		"mul	r6,r1,r3\n\t"
		"adds	r5,r5,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#14\n\t"
		"adcs	r5,r5,#120\n\t"

		"strh	r4,[r9],#2\n\t" /* v_out.x */
		"strh	r5,[r9],#2\n\t" /* v_out.y */

		"cmp	r8,r10\n\t"
		"bne	21b\n\t"

		/* preload cos(idx * 4) */
		"lsls	r0,%[idx],#2\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx * 4) */
		"lsls	r0,%[idx],#2\n\t"
		"bl	sin15\n\t"

		"ldr	r8,=tri_obj_0\n\t"
	"22:\n\t"
		"ldrsh	r2,[r8],#2\n\t" /* v_in.x */
		"ldrsh	r3,[r8],#2\n\t" /* v_in.y */

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r4,r1,r2\n\t"
		"mul	r6,r0,r3\n\t"
		"subs	r4,r4,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r4,r4,#13\n\t"
		"adcs	r4,r4,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r5,r0,r2\n\t"
		"mul	r6,r1,r3\n\t"
		"adds	r5,r5,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#13\n\t"
		"adcs	r5,r5,#120\n\t"

		"strh	r4,[r9],#2\n\t" /* v_out.x */
		"strh	r5,[r9],#2\n\t" /* v_out.y */

		"cmp	r8,r10\n\t"
		"bne	22b\n\t"

		/* scan-convert the scr-space tri edges */
		"movs	r11,r9\n\t"
		"ldr	r8,[sp]\n\t"
		"mvns	r8,r8\n\t"
	"3:\n\t"
		"ldrsh	r0,[r10,#tri_p0+R2_x]\n\t"
		"ldrsh	r1,[r10,#tri_p0+R2_y]\n\t"
		"ldrsh	r2,[r10,#tri_p1+R2_x]\n\t"
		"ldrsh	r3,[r10,#tri_p1+R2_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line\n\t"

		"ldrsh	r0,[r10,#tri_p1+R2_x]\n\t"
		"ldrsh	r1,[r10,#tri_p1+R2_y]\n\t"
		"ldrsh	r2,[r10,#tri_p2+R2_x]\n\t"
		"ldrsh	r3,[r10,#tri_p2+R2_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line\n\t"

		"ldrsh	r0,[r10,#tri_p2+R2_x]\n\t"
		"ldrsh	r1,[r10,#tri_p2+R2_y]\n\t"
		"ldrsh	r2,[r10,#tri_p0+R2_x]\n\t"
		"ldrsh	r3,[r10,#tri_p0+R2_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line\n\t"

		"adds	r10,r10,#tri_size\n\t"
		"cmp	r10,r11\n\t"
		"bne	3b\n\t"

		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_rot2d_solid_triforce(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse solid triangles rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R2 */
	".equ	R2_x, R_size * 0\n\t" /* short */
	".equ	R2_y, R_size * 1\n\t" /* short */
	".equ	R2_size, R_size * 2\n\t"

	/* struct tri */
	".equ	tri_p0, R2_size * 0\n\t" /* R2 */
	".equ	tri_p1, R2_size * 1\n\t" /* R2 */
	".equ	tri_p2, R2_size * 2\n\t" /* R2 */
	".equ	tri_size, R2_size * 3\n\t"

	/* struct pb (parallelogram basis) */
	".equ	pb_e01,  R2_size * 0\n\t" /* R2 */
	".equ	pb_e02,  R2_size * 1\n\t" /* R2 */
	".equ	pb_area, R2_size * 2\n\t" /* word */
	".equ	pb_size, R2_size * 2 + 4\n\t"

	/* plot tris */
	/* transform obj -> scr */
		/* preload cos(idx * 4) */
		"lsls	r0,%[idx],#2\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx * 4) */
		"lsls	r0,%[idx],#2\n\t"
		"bl	sin15\n\t"

		"ldr	r8,=tri_obj_0\n\t"
		"adds	r9,r8,#tri_size\n\t"
		"movs	r10,r9\n\t"
	"20:\n\t"
		"ldrsh	r2,[r8],#2\n\t" /* v_in.x */
		"ldrsh	r3,[r8],#2\n\t" /* v_in.y */

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r4,r1,r2\n\t"
		"mul	r6,r0,r3\n\t"
		"subs	r4,r4,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r4,r4,#13\n\t"
		"adcs	r4,r4,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r5,r0,r2\n\t"
		"mul 	r6,r1,r3\n\t"
		"adds	r5,r5,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#13\n\t"
		"adcs	r5,r5,#120\n\t"

		"strh	r4,[r9],#2\n\t" /* v_out.x */
		"strh	r5,[r9],#2\n\t" /* v_out.y */

		"cmp	r8,r10\n\t"
		"bne	20b\n\t"

		/* preload cos(idx * 2) */
		"lsls	r0,%[idx],#1\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx * 2) */
		"lsls	r0,%[idx],#1\n\t"
		"bl	sin15\n\t"

		"ldr	r8,=tri_obj_0\n\t"
	"21:\n\t"
		"ldrsh	r2,[r8],#2\n\t" /* v_in.x */
		"ldrsh	r3,[r8],#2\n\t" /* v_in.y */

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r4,r1,r2\n\t"
		"mul	r6,r0,r3\n\t"
		"subs	r4,r4,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r4,r4,#14\n\t"
		"adcs	r4,r4,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r5,r0,r2\n\t"
		"mul	r6,r1,r3\n\t"
		"adds	r5,r5,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#14\n\t"
		"adcs	r5,r5,#120\n\t"

		"strh	r4,[r9],#2\n\t" /* v_out.x */
		"strh	r5,[r9],#2\n\t" /* v_out.y */

		"cmp	r8,r10\n\t"
		"bne	21b\n\t"

		/* preload cos(idx * 1) */
		"lsls	r0,%[idx],#0\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx * 1) */
		"lsls	r0,%[idx],#0\n\t"
		"bl	sin15\n\t"

		"ldr	r8,=tri_obj_0\n\t"
	"22:\n\t"
		"ldrsh	r2,[r8],#2\n\t" /* v_in.x */
		"ldrsh	r3,[r8],#2\n\t" /* v_in.y */

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r4,r1,r2\n\t"
		"mul	r6,r0,r3\n\t"
		"subs	r4,r4,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r5,r0,r2\n\t"
		"mul	r6,r1,r3\n\t"
		"adds	r5,r5,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#120\n\t"

		"strh	r4,[r9],#2\n\t" /* v_out.x */
		"strh	r5,[r9],#2\n\t" /* v_out.y */

		"cmp	r8,r10\n\t"
		"bne	22b\n\t"

		/* scan-convert the scr-space tri */
	"3:\n\t"
		"subs	r12,sp,#pb_size\n\t"
		/* compute parallelogram basis */
		"ldrsh	r0,[r10],#2\n\t"
		"ldrsh	r1,[r10],#2\n\t"
		"ldrh	r2,[r10],#2\n\t"
		"ldrh	r3,[r10],#2\n\t"
		"ldrh	r4,[r10],#2\n\t"
		"ldrh	r5,[r10],#2\n\t"
		"bl	init_pb\n\t"
		/* skip tris of negative or zero area */
		"ble	7f\n\t"

		"ldrsh	r2,[r10,#-8]\n\t"
		"ldrsh	r3,[r10,#-6]\n\t"
		"ldrsh	r4,[r10,#-4]\n\t"
		"ldrsh	r5,[r10,#-2]\n\t"

		/* compute tri AABB */
		/* ascending sort in x-direction */
		"movs	r6,r0\n\t"
		"movs	r7,r2\n\t"
		"movs	r8,r4\n\t"

		"cmp	r6,r7\n\t"
		"ble	31f\n\t"

		"movs	r6,r2\n\t"
		"movs	r7,r0\n\t"
	"31:\n\t"
		"cmp	r6,r8\n\t"
		"ble	32f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"32:\n\t"
		"cmp	r7,r8\n\t"
		"ble	33f\n\t"

		"movs	r8,r7\n\t"
	"33:\n\t"
#if 0
		/* intersect tri x-span with fb x-span */
		"cmp	r6,#0\n\t"
		"bge	331f\n\t"
		"movs	r6,#0\n\t"
	"331:\n\t"
		"cmp	r8,#320\n\t"
		"blt	332f\n\t"
		"movw	r8,#320-1\n\t"
	"332:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"
#endif
		/* store x_min, preload x_min and x_max */
		"str	r6,[r12,#-4]\n\t"
		"movs	r11,r6\n\t"
		"movs	r14,r8\n\t"

		/* ascending sort in y-direction */
		"movs	r6,r1\n\t"
		"movs	r7,r3\n\t"
		"movs	r8,r5\n\t"

		"cmp	r6,r7\n\t"
		"ble	34f\n\t"

		"movs	r6,r3\n\t"
		"movs	r7,r1\n\t"
	"34:\n\t"
		"cmp	r6,r8\n\t"
		"ble	35f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"35:\n\t"
		"cmp	r7,r8\n\t"
		"ble	36f\n\t"

		"movs	r8,r7\n\t"
	"36:\n\t"
#if 0
		/* intersect tri y-span with fb y-span */
		"cmp	r6,#0\n\t"
		"bge	361f\n\t"
		"movs	r6,#0\n\t"
	"361:\n\t"
		"cmp	r8,#240\n\t"
		"blt	362f\n\t"
		"movs	r8,#240-1\n\t"
	"362:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"
#endif
		/* store y_max, preload y_min */
		"str 	r8,[r12,#-8]\n\t"
		"movs	r7,r6\n\t"

		/* preload p0 */
		"movs	r2,r0\n\t"
		"movs	r3,r1\n\t"
		/* preload pb.e01, pb.e02, pb.area */
		"ldm	r12,{r4-r6}\n\t"

		/* preload color and fb ptr */
		"ldr	r8,[sp]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"mvns	r8,r8\n\t"

		"stmdb	sp!,{r9-r10}\n\t" /* att: room for pb_size bytes */

		/* iterate AABB along y */
		"movs	r9,#640\n\t"
		"mla	%[fb],r7,r9,%[fb]\n\t"
	"4:\n\t"
		/* iterate AABB along x */
	"5:\n\t"
		/* get barycentric coords for x,y */
		/* see barycentric.s:get_coord */
		"subs	r0,r11,r2\n\t"
		"subs	r1,r7,r3\n\t"

		"movs	r9,r0\n\t"
		"movs	r10,r1\n\t"

		"smulbt	r0,r0,r5\n\t"
		"smulbb	r1,r1,r4\n\t"
		"smulbt	r9,r9,r4\n\t"
		"smulbb	r10,r10,r5\n\t"

		/* if {s|t} < 0 || (s+t) > pb.area then pixel is outside */
		"subs	r1,r1,r9\n\t"
		"blt	6f\n\t"
		"subs	r0,r0,r10\n\t"
		"blt	6f\n\t"

		"adds	r0,r0,r1\n\t"
		"cmp	r0,r6\n\t"
		"bgt	6f\n\t"
		/* plot pixel */
		"strh	r8,[%[fb],r11,lsl #1]\n\t"
	"6:\n\t"
		"adds	r11,r11,#1\n\t"
		"cmp	r11,r14\n\t"
		"bls	5b\n\t"

		"ldrd	r1,r11,[sp,#-8-pb_size+8]\n\t"
		"adds	%[fb],%[fb],#640\n\t"
		"adds	r7,r7,#1\n\t"
		"cmp	r7,r1\n\t"
		"bls	4b\n\t"

		"ldmia	sp!,{r9-r10}\n\t"
		/* invert color */
		"str	r8,[sp]\n\t"
	"7:\n\t"
		"cmp	r10,r9\n\t"
		"bne	3b\n\t"

		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

uint16_t mask_shift[2] __attribute__ ((aligned(4)));

static void alt_rot2d_solid_tri(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Color gradient solid triangle rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R2 */
	".equ	R2_x, R_size * 0\n\t" /* short */
	".equ	R2_y, R_size * 1\n\t" /* short */
	".equ	R2_size, R_size * 2\n\t"

	/* struct tri */
	".equ	tri_p0, R2_size * 0\n\t" /* R2 */
	".equ	tri_p1, R2_size * 1\n\t" /* R2 */
	".equ	tri_p2, R2_size * 2\n\t" /* R2 */
	".equ	tri_size, R2_size * 3\n\t"

	/* struct pb (parallelogram basis) */
	".equ	pb_e01,  R2_size * 0\n\t" /* R2 */
	".equ	pb_e02,  R2_size * 1\n\t" /* R2 */
	".equ	pb_area, R2_size * 2\n\t" /* word */
	".equ	pb_size, R2_size * 2 + 4\n\t"

	/* plot tris */
	/* transform obj -> scr */
		/* preload cos(idx * 4) */
		"movs	r0,%[idx]\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx * 4) */
		"movs	r0,%[idx]\n\t"
		"bl	sin15\n\t"

		"ldr	r8,=tri_obj_0\n\t"
		"adds	r9,r8,#tri_size\n\t"
		"movs	r10,r9\n\t"
	"2:\n\t"
		"ldrsh	r2,[r8],#2\n\t" /* v_in.x */
		"ldrsh	r3,[r8],#2\n\t" /* v_in.y */

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r4,r1,r2\n\t"
		"mul	r6,r0,r3\n\t"
		"subs	r4,r4,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r4,r4,#13\n\t"
		"adcs	r4,r4,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r5,r0,r2\n\t"
		"mul 	r6,r1,r3\n\t"
		"adds	r5,r5,r6\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#13\n\t"
		"adcs	r5,r5,#120\n\t"

		"strh	r4,[r9],#2\n\t" /* v_out.x */
		"strh	r5,[r9],#2\n\t" /* v_out.y */

		"cmp	r8,r10\n\t"
		"bne	2b\n\t"

		/* scan-convert the scr-space tri */
		"subs	sp,sp,#8+pb_size\n\t"
	"3:\n\t"
		"adds	r12,sp,#8\n\t"
		/* compute parallelogram basis */
		"ldrsh	r0,[r10],#2\n\t"
		"ldrsh	r1,[r10],#2\n\t"
		"ldrh	r2,[r10],#2\n\t"
		"ldrh	r3,[r10],#2\n\t"
		"ldrh	r4,[r10],#2\n\t"
		"ldrh	r5,[r10],#2\n\t"
		"bl	init_pb\n\t"
		/* skip tris of negative or zero area */
		"ble	7f\n\t"

		"ldrsh	r2,[r10,#-8]\n\t"
		"ldrsh	r3,[r10,#-6]\n\t"
		"ldrsh	r4,[r10,#-4]\n\t"
		"ldrsh	r5,[r10,#-2]\n\t"

		/* compute tri AABB */
		/* ascending sort in x-direction */
		"movs	r6,r0\n\t"
		"movs	r7,r2\n\t"
		"movs	r8,r4\n\t"

		"cmp	r6,r7\n\t"
		"ble	31f\n\t"

		"movs	r6,r2\n\t"
		"movs	r7,r0\n\t"
	"31:\n\t"
		"cmp	r6,r8\n\t"
		"ble	32f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"32:\n\t"
		"cmp	r7,r8\n\t"
		"ble	33f\n\t"

		"movs	r8,r7\n\t"
	"33:\n\t"
#if 0
		/* intersect tri x-span with fb x-span */
		"cmp	r6,#0\n\t"
		"bge	331f\n\t"
		"movs	r6,#0\n\t"
	"331:\n\t"
		"cmp	r8,#320\n\t"
		"blt	332f\n\t"
		"movw	r8,#320-1\n\t"
	"332:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"
#endif
		/* store x_min, preload x_min and x_max */
		"str	r6,[sp,#4]\n\t"
		"movs	r11,r6\n\t"
		"movs	r14,r8\n\t"

		/* ascending sort in y-direction */
		"movs	r6,r1\n\t"
		"movs	r7,r3\n\t"
		"movs	r8,r5\n\t"

		"cmp	r6,r7\n\t"
		"ble	34f\n\t"

		"movs	r6,r3\n\t"
		"movs	r7,r1\n\t"
	"34:\n\t"
		"cmp	r6,r8\n\t"
		"ble	35f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"35:\n\t"
		"cmp	r7,r8\n\t"
		"ble	36f\n\t"

		"movs	r8,r7\n\t"
	"36:\n\t"
#if 0
		/* intersect tri y-span with fb y-span */
		"cmp	r6,#0\n\t"
		"bge	361f\n\t"
		"movs	r6,#0\n\t"
	"361:\n\t"
		"cmp	r8,#240\n\t"
		"blt	362f\n\t"
		"movs	r8,#240-1\n\t"
	"362:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"
#endif
		/* store y_max, preload y_min */
		"str 	r8,[sp,#0]\n\t"
		"movs	r7,r6\n\t"

		/* preload p0 */
		"movs	r2,r0\n\t"
		"movs	r3,r1\n\t"
		/* preload pb.e01, pb.e02, pb.area */
		"ldm	r12,{r4-r6}\n\t"

		/* preload fb ptr */
		"ldr	%[fb],[sp,#8+pb_size+8]\n\t"

		"stmdb	sp!,{r9-r10}\n\t" /* att: room for pb_size bytes */

		/* iterate AABB along y */
		"ldr	r10,=mask_shift\n\t"
		"ldr	r10,[r10]\n\t"
		"movs	r9,#640\n\t"
		"mla	%[fb],r7,r9,%[fb]\n\t"
	"4:\n\t"
		/* iterate AABB along x */
	"5:\n\t"
		/* get barycentric coords for x,y */
		/* see barycentric.s:get_coord */
		"subs	r0,r11,r2\n\t"
		"subs	r1,r7,r3\n\t"

		"movs	r8,r0\n\t"
		"movs	r9,r1\n\t"

		"smulbt	r0,r0,r5\n\t"
		"smulbb	r1,r1,r4\n\t"
		"smulbt	r8,r8,r4\n\t"
		"smulbb	r9,r9,r5\n\t"

		/* if {s|t} < 0 || (s+t) > pb.area then pixel is outside */
		"subs	r1,r1,r8\n\t"
		"blt	6f\n\t"
		"subs	r0,r0,r9\n\t"
		"blt	6f\n\t"

		"adds	r8,r0,r1\n\t"
		"subs	r8,r6\n\t"
		"bgt	6f\n\t"

		"negs	r8,r8\n\t"
		/* use barycentric U for color at p0; p1 & p2 assumed black */
		"uxth	r0,r10\n\t"
		"lsrs	r1,r10,#16\n\t"
		"mul	r0,r0,r8\n\t"
		"sdiv	r0,r0,r6\n\t"
		/* plot pixel */
		"lsls	r0,r0,r1\n\t"
		"strh	r0,[%[fb],r11,lsl #1]\n\t"
	"6:\n\t"
		"adds	r11,r11,#1\n\t"
		"cmp	r11,r14\n\t"
		"bls	5b\n\t"

		"ldrd	r1,r11,[sp,#8]\n\t"
		"adds	%[fb],%[fb],#640\n\t"
		"adds	r7,r7,#1\n\t"
		"cmp	r7,r1\n\t"
		"bls	4b\n\t"

		"ldmia	sp!,{r9-r10}\n\t"
	"7:\n\t"
		"cmp	r10,r9\n\t"
		"bne	3b\n\t"

		"adds	sp,sp,#8+pb_size\n\t"
		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_rot2d_wire_trilist_bc(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse backface-culled wireframe trilist z-rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R3 */
	".equ	R3_x, R_size * 0\n\t" /* short */
	".equ	R3_y, R_size * 1\n\t" /* short */
	".equ	R3_z, R_size * 2\n\t" /* short */
	".equ	R3_size, R_size * 3\n\t"

	/* struct tri */
	".equ	tri_p0, R3_size * 0\n\t" /* R3 */
	".equ	tri_p1, R3_size * 1\n\t" /* R3 */
	".equ	tri_p2, R3_size * 2\n\t" /* R3 */
	".equ	tri_size, R3_size * 3\n\t"

	/* struct pb (parallelogram basis) */
	".equ	pb_e01,  R2_size * 0\n\t" /* R2 */
	".equ	pb_e02,  R2_size * 1\n\t" /* R2 */
	".equ	pb_area, R2_size * 2\n\t" /* word */
	".equ	pb_size, R2_size * 2 + 4\n\t"

	/* plot tris */
	/* transform obj -> scr */
		/* preload cos(idx) */
		"movs	r0,%[idx]\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx) */
		"movs	r0,%[idx]\n\t"
		"bl	sin15\n\t"

		"ldr	r12,=mesh_obj\n\t"
		"ldr	r12,[r12]\n\t"
		"ldrh	r14,[r12],#2\n\t"
		"movs	r2,#tri_size\n\t"
		"mul	r14,r14,r2\n\t"
		"adds	r11,r14,r12\n\t"
		"ldr	r14,=mesh_scr\n\t"
	"2:\n\t"
		"ldrsh	r2,[r12,#R3_x]\n\t" /* v_in.x */
		"ldrsh	r3,[r12,#R3_y]\n\t" /* v_in.y */
		"adds	r12,r12,#R3_size\n\t"

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r5,r1,r2\n\t"
		"mul	r10,r0,r3\n\t"
		"subs	r5,r5,r10\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r6,r0,r2\n\t"
		"mul 	r10,r1,r3\n\t"
		"adds	r6,r6,r10\n\t"

		/* fx16.15 -> int16 */
		"asrs	r6,r6,#15\n\t"
		"adcs	r6,r6,#120\n\t"

		"strh	r5,[r14,#R3_x]\n\t" /* v_out.x */
		"strh	r6,[r14,#R3_y]\n\t" /* v_out.y */
		"adds	r14,r14,#R3_size\n\t"

		"cmp	r11,r12\n\t"
		"bne	2b\n\t"

		/* scan-convert the scr-space tri edges */
		"movs	r10,r14\n\t"
		"ldr	r11,=mesh_scr\n\t"
		"ldr	r8,[sp]\n\t"
		"mvns	r8,r8\n\t"
	"3:\n\t"
		"subs	r12,sp,#pb_size\n\t"
		"ldrh	r0,[r11,#tri_p0+R3_x]\n\t"
		"ldrh	r1,[r11,#tri_p0+R3_y]\n\t"
		"ldrh	r2,[r11,#tri_p1+R3_x]\n\t"
		"ldrh	r3,[r11,#tri_p1+R3_y]\n\t"
		"ldrh	r4,[r11,#tri_p2+R3_x]\n\t"
		"ldrh	r5,[r11,#tri_p2+R3_y]\n\t"
		"bl	init_pb\n\t"
		/* skip tris of negative or zero area */
		"ble	4f\n\t"

		"ldrsh	r0,[r11,#tri_p0+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p0+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p1+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p1+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"

		"ldrsh	r0,[r11,#tri_p1+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p1+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p2+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p2+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"

		"ldrsh	r0,[r11,#tri_p2+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p2+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p0+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p0+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"
	"4:\n\t"
		"adds	r11,r11,#tri_size\n\t"
		"cmp	r10,r11\n\t"
		"bne	3b\n\t"

		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_rot2d_solid_trilist_bc(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse backface-culled solid trilist z-rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R3 */
	".equ	R3_x, R_size * 0\n\t" /* short */
	".equ	R3_y, R_size * 1\n\t" /* short */
	".equ	R3_z, R_size * 2\n\t" /* short */
	".equ	R3_size, R_size * 3\n\t"

	/* struct tri */
	".equ	tri_p0, R3_size * 0\n\t" /* R3 */
	".equ	tri_p1, R3_size * 1\n\t" /* R3 */
	".equ	tri_p2, R3_size * 2\n\t" /* R3 */
	".equ	tri_size, R3_size * 3\n\t"

	/* struct pb (parallelogram basis) */
	".equ	pb_e01,  R2_size * 0\n\t" /* R2 */
	".equ	pb_e02,  R2_size * 1\n\t" /* R2 */
	".equ	pb_area, R2_size * 2\n\t" /* word */
	".equ	pb_size, R2_size * 2 + 4\n\t"

	/* plot tris */
	/* transform obj -> scr */
		/* preload cos(idx) */
		"movs	r0,%[idx]\n\t"
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		/* preload sin(idx) */
		"movs	r0,%[idx]\n\t"
		"bl	sin15\n\t"

		"ldr	r12,=mesh_obj\n\t"
		"ldr	r12,[r12]\n\t"
		"ldrh	r14,[r12],#2\n\t"
		"movs	r2,#tri_size\n\t"
		"mul	r14,r14,r2\n\t"
		"adds	r11,r14,r12\n\t"
		"ldr	r14,=mesh_scr\n\t"
	"2:\n\t"
		"ldrsh	r2,[r12,#R3_x]\n\t" /* v_in.x */
		"ldrsh	r3,[r12,#R3_y]\n\t" /* v_in.y */
		"adds	r12,r12,#R3_size\n\t"

		/* transform vertex x-coord: cos * x - sin * y */
		"mul	r5,r1,r2\n\t"
		"mul	r10,r0,r3\n\t"
		"subs	r5,r5,r10\n\t"

		/* fx16.15 -> int16 */
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#160\n\t"

		/* transform vertex y-coord: sin * x + cos * y */
		"mul	r6,r0,r2\n\t"
		"mul 	r10,r1,r3\n\t"
		"adds	r6,r6,r10\n\t"

		/* fx16.15 -> int16 */
		"asrs	r6,r6,#15\n\t"
		"adcs	r6,r6,#120\n\t"

		"strh	r5,[r14,#R3_x]\n\t" /* v_out.x */
		"strh	r6,[r14,#R3_y]\n\t" /* v_out.y */
		"adds	r14,r14,#R3_size\n\t"

		"cmp	r11,r12\n\t"
		"bne	2b\n\t"

		/* scan-convert the scr-space tris */
		"ldr	r10,=mesh_scr\n\t"
		"movs	r9,r14\n\t"
	"3:\n\t"
		"subs	r12,sp,#pb_size\n\t"
		/* compute parallelogram basis */
		"ldrsh	r0,[r10,#tri_p0+R3_x]\n\t"
		"ldrsh	r1,[r10,#tri_p0+R3_y]\n\t"
		"ldrh	r2,[r10,#tri_p1+R3_x]\n\t"
		"ldrh	r3,[r10,#tri_p1+R3_y]\n\t"
		"ldrh	r4,[r10,#tri_p2+R3_x]\n\t"
		"ldrh	r5,[r10,#tri_p2+R3_y]\n\t"
		"bl	init_pb\n\t"
		/* skip tris of negative or zero area */
		"ble	7f\n\t"

		"ldrsh	r2,[r10,#tri_p1+R3_x]\n\t"
		"ldrsh	r3,[r10,#tri_p1+R3_y]\n\t"
		"ldrsh	r4,[r10,#tri_p2+R3_x]\n\t"
		"ldrsh	r5,[r10,#tri_p2+R3_y]\n\t"

		/* compute tri AABB */
		/* ascending sort in x-direction */
		"movs	r6,r0\n\t"
		"movs	r7,r2\n\t"
		"movs	r8,r4\n\t"

		"cmp	r6,r7\n\t"
		"ble	31f\n\t"

		"movs	r6,r2\n\t"
		"movs	r7,r0\n\t"
	"31:\n\t"
		"cmp	r6,r8\n\t"
		"ble	32f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"32:\n\t"
		"cmp	r7,r8\n\t"
		"ble	33f\n\t"

		"movs	r8,r7\n\t"
	"33:\n\t"
		/* intersect tri x-span with fb x-span */
		"cmp	r6,#0\n\t"
		"bge	331f\n\t"
		"movs	r6,#0\n\t"
	"331:\n\t"
		"cmp	r8,#320\n\t"
		"blt	332f\n\t"
		"movw	r8,#320-1\n\t"
	"332:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"

		/* store x_min, preload x_min and x_max */
		"str	r6,[r12,#-4]\n\t"
		"movs	r11,r6\n\t"
		"movs	r14,r8\n\t"

		/* ascending sort in y-direction */
		"movs	r6,r1\n\t"
		"movs	r7,r3\n\t"
		"movs	r8,r5\n\t"

		"cmp	r6,r7\n\t"
		"ble	34f\n\t"

		"movs	r6,r3\n\t"
		"movs	r7,r1\n\t"
	"34:\n\t"
		"cmp	r6,r8\n\t"
		"ble	35f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"35:\n\t"
		"cmp	r7,r8\n\t"
		"ble	36f\n\t"

		"movs	r8,r7\n\t"
	"36:\n\t"
		/* intersect tri y-span with fb y-span */
		"cmp	r6,#0\n\t"
		"bge	361f\n\t"
		"movs	r6,#0\n\t"
	"361:\n\t"
		"cmp	r8,#240\n\t"
		"blt	362f\n\t"
		"movs	r8,#240-1\n\t"
	"362:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"

		/* store y_max, preload y_min */
		"str	r8,[r12,#-8]\n\t"
		"movs	r7,r6\n\t"

		/* preload p0 */
		"movs	r2,r0\n\t"
		"movs	r3,r1\n\t"
		/* preload pb.e01, pb.e02, pb.area */
		"ldm	r12,{r4-r6}\n\t"

		/* preload color and fb ptr */
		"ldr	r8,[sp]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"mvns	r8,r8\n\t"

		"stmdb	sp!,{r9-r10}\n\t" /* att: room for pb_size bytes */

		/* iterate AABB along y */
		"movs	r9,#640\n\t"
		"mla	%[fb],r7,r9,%[fb]\n\t"
	"4:\n\t"
		/* iterate AABB along x */
	"5:\n\t"
		/* get barycentric coords for x,y */
		/* see barycentric.s:get_coord */
		"subs	r0,r11,r2\n\t"
		"subs	r1,r7,r3\n\t"

		"movs	r9,r0\n\t"
		"movs	r10,r1\n\t"

		"smulbt	r0,r0,r5\n\t"
		"smulbb	r1,r1,r4\n\t"
		"smulbt	r9,r9,r4\n\t"
		"smulbb	r10,r10,r5\n\t"

		/* if {s|t} < 0 || (s+t) > pb.area then pixel is outside */
		"subs	r1,r1,r9\n\t"
		"blt	6f\n\t"
		"subs	r0,r0,r10\n\t"
		"blt	6f\n\t"

		"adds	r0,r0,r1\n\t"
		"cmp	r0,r6\n\t"
		"bgt	6f\n\t"
		/* plot pixel */
		"strh	r8,[%[fb],r11,lsl #1]\n\t"
	"6:\n\t"
		"adds	r11,r11,#1\n\t"
		"cmp	r11,r14\n\t"
		"bls	5b\n\t"

		"ldrd	r1,r11,[sp,#-8-pb_size+8]\n\t"
		"adds	%[fb],%[fb],#640\n\t"
		"adds	r7,r7,#1\n\t"
		"cmp	r7,r1\n\t"
		"bls	4b\n\t"

		"ldmia	sp!,{r9-r10}\n\t"
	"7:\n\t"
		"adds	r10,r10,#tri_size\n\t"
		"cmp	r10,r9\n\t"
		"bne	3b\n\t"

		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_rot3d_wire_trilist(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse wireframe trilist rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R3 */
	".equ	R3_x, R_size * 0\n\t" /* short */
	".equ	R3_y, R_size * 1\n\t" /* short */
	".equ	R3_z, R_size * 2\n\t" /* short */
	".equ	R3_size, R_size * 3\n\t"

	/* struct tri */
	".equ	tri_p0, R3_size * 0\n\t" /* R3 */
	".equ	tri_p1, R3_size * 1\n\t" /* R3 */
	".equ	tri_p2, R3_size * 2\n\t" /* R3 */
	".equ	tri_size, R3_size * 3\n\t"

	/* matrix element, aka E */
	".equ	E_size, 4\n\t" /* word */

	/* struct row */
	".equ	row_x, E_size * 0\n\t" /* word */
	".equ	row_y, E_size * 1\n\t" /* word */
	".equ	row_z, E_size * 2\n\t" /* word */
	".equ	row_size, E_size * 3\n\t"

	/* struct mat */
	".equ	mat_0, row_size * 0\n\t" /* row */
	".equ	mat_1, row_size * 1\n\t" /* row */
	".equ	mat_2, row_size * 2\n\t" /* row */
	".equ	mat_size, row_size * 3\n\t"

	/* plot tris */
	/* transform obj -> scr */
		"movs	r0,#0x30\n\t" /* r10: cosB */
		"bl	cos15\n\t"
		"movs	r10,r0\n\t"
		"movs	r0,#0x30\n\t" /* r9: sinB */
		"bl	sin15\n\t"
		"movs	r9,r0\n\t"

		"movs	r0,%[idx]\n\t" /* r1: cosA */
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		"movs	r0,%[idx]\n\t" /* r0: sinA */
		"bl	sin15\n\t"

		/* preload mat_{0,1,2} */
		"movs	r3,r1\n\t"	/* m00: cosA */
		"mul	r4,r0,r10\n\t"	/* m01: sinA cosB */
		"mul	r5,r0,r9\n\t"	/* m02: sinA sinB */

		"negs	r6,r0\n\t"	/* m10: -sinA */
		"mul	r7,r1,r10\n\t"	/* m11: cosA cosB */
		"mul	r8,r1,r9\n\t"	/* m12: cosA sinB */

		"movs	r11,r10\n\t"	/* m22: cosB */
		"negs	r10,r9\n\t"	/* m21: -sinB */
		"movs	r9,#0\n\t"	/* m20: 0 */

		/* fx2.30 -> fx17.15 */
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#0\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"
		"asrs	r7,r7,#15\n\t"
		"adcs	r7,r7,#0\n\t"
		"asrs	r8,r8,#15\n\t"
		"adcs	r8,r8,#0\n\t"

		/* store mat_0 */
		"stmdb	sp!,{r3-r5}\n\t"

		"ldr	r12,=mesh_obj\n\t"
		"ldr	r12,[r12]\n\t"
		"ldrh	r14,[r12],#2\n\t"
		"movs	r0,#tri_size\n\t"
		"mul	r14,r14,r0\n\t"
		"adds	r14,r14,r12\n\t"

		/* store mesh_obj_end */
		"stmdb	sp!,{r14}\n\t"
		"ldr	r14,=mesh_scr\n\t"
	"2:\n\t"
		"ldrsh	r0,[r12],#2\n\t" /* v_in.x */
		"ldrsh	r1,[r12],#2\n\t" /* v_in.y */
		"ldrsh	r2,[r12],#2\n\t" /* v_in.z */

		/* inline mul_vec3_mat to reduce reg pressure */
		"muls	r3,r3,r0\n\t"
		"muls	r4,r4,r0\n\t"
		"muls	r5,r5,r0\n\t"

		"mla	r3,r6,r1,r3\n\t"
		"mla	r4,r7,r1,r4\n\t"
		"mla	r5,r8,r1,r5\n\t"

		"mla	r3,r9, r2,r3\n\t"
		"mla	r4,r10,r2,r4\n\t"
		"mla	r5,r11,r2,r5\n\t"

		/* fx16.15 -> int16 */
		"asrs	r3,r3,#15\n\t"
		"adcs	r3,r3,#160\n\t"
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#120\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"

		"strh	r3,[r14],#2\n\t" /* v_out.x */
		"strh	r4,[r14],#2\n\t" /* v_out.y */
		"strh	r5,[r14],#2\n\t" /* v_out.z */

		/* restore mesh_obj_end and mat_0 */
		"ldm	sp,{r0,r3-r5}\n\t"

		"cmp	r0,r12\n\t"
		"bne	2b\n\t"

		"adds	sp,sp,#4 * 4\n\t"

		/* scan-convert the scr-space tri edges */
		"ldr	r11,=mesh_scr\n\t"
		"movs	r10,r14\n\t"
		"ldr	r8,[sp]\n\t"
		"mvns	r8,r8\n\t"
	"3:\n\t"
		"ldrsh	r0,[r11,#tri_p0+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p0+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p1+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p1+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"

		"ldrsh	r0,[r11,#tri_p1+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p1+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p2+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p2+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"

		"ldrsh	r0,[r11,#tri_p2+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p2+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p0+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p0+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"

		"adds	r11,r11,#tri_size\n\t"
		"cmp	r10,r11\n\t"
		"bne	3b\n\t"

		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_rot3d_wire_trilist_bc(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse backface-culled wireframe trilist rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R3 */
	".equ	R3_x, R_size * 0\n\t" /* short */
	".equ	R3_y, R_size * 1\n\t" /* short */
	".equ	R3_z, R_size * 2\n\t" /* short */
	".equ	R3_size, R_size * 3\n\t"

	/* struct tri */
	".equ	tri_p0, R3_size * 0\n\t" /* R3 */
	".equ	tri_p1, R3_size * 1\n\t" /* R3 */
	".equ	tri_p2, R3_size * 2\n\t" /* R3 */
	".equ	tri_size, R3_size * 3\n\t"

	/* matrix element, aka E */
	".equ	E_size, 4\n\t" /* word */

	/* struct row */
	".equ	row_x, E_size * 0\n\t" /* word */
	".equ	row_y, E_size * 1\n\t" /* word */
	".equ	row_z, E_size * 2\n\t" /* word */
	".equ	row_size, E_size * 3\n\t"

	/* struct mat */
	".equ	mat_0, row_size * 0\n\t" /* row */
	".equ	mat_1, row_size * 1\n\t" /* row */
	".equ	mat_2, row_size * 2\n\t" /* row */
	".equ	mat_size, row_size * 3\n\t"

	/* struct pb (parallelogram basis) */
	".equ	pb_e01,  R2_size * 0\n\t" /* R2 */
	".equ	pb_e02,  R2_size * 1\n\t" /* R2 */
	".equ	pb_area, R2_size * 2\n\t" /* word */
	".equ	pb_size, R2_size * 2 + 4\n\t"

	/* plot tris */
	/* transform obj -> scr */
		"movs	r0,#0x30\n\t" /* r10: cosB */
		"bl	cos15\n\t"
		"movs	r10,r0\n\t"
		"movs	r0,#0x30\n\t" /* r9: sinB */
		"bl	sin15\n\t"
		"movs	r9,r0\n\t"

		"movs	r0,%[idx]\n\t" /* r1: cosA */
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		"movs	r0,%[idx]\n\t" /* r0: sinA */
		"bl	sin15\n\t"

		/* preload mat_{0,1,2} */
		"movs	r3,r1\n\t"	/* m00: cosA */
		"mul	r4,r0,r10\n\t"	/* m01: sinA cosB */
		"mul	r5,r0,r9\n\t"	/* m02: sinA sinB */

		"negs	r6,r0\n\t"	/* m10: -sinA */
		"mul	r7,r1,r10\n\t"	/* m11: cosA cosB */
		"mul	r8,r1,r9\n\t"	/* m12: cosA sinB */

		"movs	r11,r10\n\t"	/* m22: cosB */
		"negs	r10,r9\n\t"	/* m21: -sinB */
		"movs	r9,#0\n\t"	/* m20: 0 */

		/* fx2.30 -> fx17.15 */
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#0\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"
		"asrs	r7,r7,#15\n\t"
		"adcs	r7,r7,#0\n\t"
		"asrs	r8,r8,#15\n\t"
		"adcs	r8,r8,#0\n\t"

		/* store mat_0 */
		"stmdb	sp!,{r3-r5}\n\t"

		"ldr	r12,=mesh_obj\n\t"
		"ldr	r12,[r12]\n\t"
		"ldrh	r14,[r12],#2\n\t"
		"movs	r0,#tri_size\n\t"
		"mul	r14,r14,r0\n\t"
		"adds	r14,r14,r12\n\t"

		/* store mesh_obj_end */
		"stmdb	sp!,{r14}\n\t"
		"ldr	r14,=mesh_scr\n\t"
	"2:\n\t"
		"ldrsh	r0,[r12],#2\n\t" /* v_in.x */
		"ldrsh	r1,[r12],#2\n\t" /* v_in.y */
		"ldrsh	r2,[r12],#2\n\t" /* v_in.z */

		/* inline mul_vec3_mat to reduce reg pressure */
		"muls	r3,r3,r0\n\t"
		"muls	r4,r4,r0\n\t"
		"muls	r5,r5,r0\n\t"

		"mla	r3,r6,r1,r3\n\t"
		"mla	r4,r7,r1,r4\n\t"
		"mla	r5,r8,r1,r5\n\t"

		"mla	r3,r9, r2,r3\n\t"
		"mla	r4,r10,r2,r4\n\t"
		"mla	r5,r11,r2,r5\n\t"

		/* fx16.15 -> int16 */
		"asrs	r3,r3,#15\n\t"
		"adcs	r3,r3,#160\n\t"
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#120\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"

		"strh	r3,[r14],#2\n\t" /* v_out.x */
		"strh	r4,[r14],#2\n\t" /* v_out.y */
		"strh	r5,[r14],#2\n\t" /* v_out.z */

		/* restore mesh_obj_end and mat_0 */
		"ldm	sp,{r0,r3-r5}\n\t"

		"cmp	r0,r12\n\t"
		"bne	2b\n\t"

		"adds	sp,sp,#4 * 4\n\t"

		/* scan-convert the scr-space tri edges */
		"ldr	r11,=mesh_scr\n\t"
		"movs	r10,r14\n\t"
		"ldr	r8,[sp]\n\t"
		"mvns	r8,r8\n\t"
	"3:\n\t"
		"subs	r12,sp,#pb_size\n\t"
		"ldrh	r0,[r11,#tri_p0+R3_x]\n\t"
		"ldrh	r1,[r11,#tri_p0+R3_y]\n\t"
		"ldrh	r2,[r11,#tri_p1+R3_x]\n\t"
		"ldrh	r3,[r11,#tri_p1+R3_y]\n\t"
		"ldrh	r4,[r11,#tri_p2+R3_x]\n\t"
		"ldrh	r5,[r11,#tri_p2+R3_y]\n\t"
		"bl	init_pb\n\t"
		/* skip tris of negative or zero area */
		"ble	4f\n\t"

		"ldrsh	r0,[r11,#tri_p0+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p0+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p1+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p1+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"

		"ldrsh	r0,[r11,#tri_p1+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p1+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p2+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p2+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"

		"ldrsh	r0,[r11,#tri_p2+R3_x]\n\t"
		"ldrsh	r1,[r11,#tri_p2+R3_y]\n\t"
		"ldrsh	r2,[r11,#tri_p0+R3_x]\n\t"
		"ldrsh	r3,[r11,#tri_p0+R3_y]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"bl	line_clip\n\t"
	"4:\n\t"
		"adds	r11,r11,#tri_size\n\t"
		"cmp	r10,r11\n\t"
		"bne	3b\n\t"

		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_rot3d_solid_trilist_bc(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse backface-culled solid trilist rotating CW on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R3 */
	".equ	R3_x, R_size * 0\n\t" /* short */
	".equ	R3_y, R_size * 1\n\t" /* short */
	".equ	R3_z, R_size * 2\n\t" /* short */
	".equ	R3_size, R_size * 3\n\t"

	/* struct tri */
	".equ	tri_p0, R3_size * 0\n\t" /* R3 */
	".equ	tri_p1, R3_size * 1\n\t" /* R3 */
	".equ	tri_p2, R3_size * 2\n\t" /* R3 */
	".equ	tri_size, R3_size * 3\n\t"

	/* matrix element, aka E */
	".equ	E_size, 4\n\t" /* word */

	/* struct row */
	".equ	row_x, E_size * 0\n\t" /* word */
	".equ	row_y, E_size * 1\n\t" /* word */
	".equ	row_z, E_size * 2\n\t" /* word */
	".equ	row_size, E_size * 3\n\t"

	/* struct mat */
	".equ	mat_0, row_size * 0\n\t" /* row */
	".equ	mat_1, row_size * 1\n\t" /* row */
	".equ	mat_2, row_size * 2\n\t" /* row */
	".equ	mat_size, row_size * 3\n\t"

	/* struct pb (parallelogram basis) */
	".equ	pb_e01,  R2_size * 0\n\t" /* R2 */
	".equ	pb_e02,  R2_size * 1\n\t" /* R2 */
	".equ	pb_area, R2_size * 2\n\t" /* word */
	".equ	pb_size, R2_size * 2 + 4\n\t"

	/* plot tris */
	/* transform obj -> scr */
		"movs	r0,#0x30\n\t" /* r10: cosB */
		"bl	cos15\n\t"
		"movs	r10,r0\n\t"
		"movs	r0,#0x30\n\t" /* r9: sinB */
		"bl	sin15\n\t"
		"movs	r9,r0\n\t"

		"movs	r0,%[idx]\n\t" /* r1: cosA */
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		"movs	r0,%[idx]\n\t" /* r0: sinA */
		"bl	sin15\n\t"

		/* preload mat_{0,1,2} */
		"movs	r3,r1\n\t"	/* m00: cosA */
		"mul	r4,r0,r10\n\t"	/* m01: sinA cosB */
		"mul	r5,r0,r9\n\t"	/* m02: sinA sinB */

		"negs	r6,r0\n\t"	/* m10: -sinA */
		"mul	r7,r1,r10\n\t"	/* m11: cosA cosB */
		"mul	r8,r1,r9\n\t"	/* m12: cosA sinB */

		"movs	r11,r10\n\t"	/* m22: cosB */
		"negs	r10,r9\n\t"	/* m21: -sinB */
		"movs	r9,#0\n\t"	/* m20: 0 */

		/* fx2.30 -> fx17.15 */
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#0\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"
		"asrs	r7,r7,#15\n\t"
		"adcs	r7,r7,#0\n\t"
		"asrs	r8,r8,#15\n\t"
		"adcs	r8,r8,#0\n\t"

		/* store mat_0 */
		"stmdb	sp!,{r3-r5}\n\t"

		"ldr	r12,=mesh_obj\n\t"
		"ldr	r12,[r12]\n\t"
		"ldrh	r14,[r12],#2\n\t"
		"movs	r0,#tri_size\n\t"
		"mul	r14,r14,r0\n\t"
		"adds	r14,r14,r12\n\t"

		/* store mesh_obj_end */
		"stmdb	sp!,{r14}\n\t"
		"ldr	r14,=mesh_scr\n\t"
	"2:\n\t"
		"ldrsh	r0,[r12],#2\n\t" /* v_in.x */
		"ldrsh	r1,[r12],#2\n\t" /* v_in.y */
		"ldrsh	r2,[r12],#2\n\t" /* v_in.z */

		/* inline mul_vec3_mat to reduce reg pressure */
		"muls	r3,r3,r0\n\t"
		"muls	r4,r4,r0\n\t"
		"muls	r5,r5,r0\n\t"

		"mla	r3,r6,r1,r3\n\t"
		"mla	r4,r7,r1,r4\n\t"
		"mla	r5,r8,r1,r5\n\t"

		"mla	r3,r9, r2,r3\n\t"
		"mla	r4,r10,r2,r4\n\t"
		"mla	r5,r11,r2,r5\n\t"

		/* fx16.15 -> int16 */
		"asrs	r3,r3,#15\n\t"
		"adcs	r3,r3,#160\n\t"
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#120\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"

		"strh	r3,[r14],#2\n\t" /* v_out.x */
		"strh	r4,[r14],#2\n\t" /* v_out.y */
		"strh	r5,[r14],#2\n\t" /* v_out.z */

		/* restore mesh_obj_end and mat_0 */
		"ldm	sp,{r0,r3-r5}\n\t"

		"cmp	r0,r12\n\t"
		"bne	2b\n\t"

		"adds	sp,sp,#4 * 4\n\t"

		/* scan-convert the scr-space tris */
		"ldr	r10,=mesh_scr\n\t"
		"movs	r9,r14\n\t"
	"3:\n\t"
		"subs	r12,sp,#pb_size\n\t"
		/* compute parallelogram basis */
		"ldrsh	r0,[r10,#tri_p0+R3_x]\n\t"
		"ldrsh	r1,[r10,#tri_p0+R3_y]\n\t"
		"ldrh	r2,[r10,#tri_p1+R3_x]\n\t"
		"ldrh	r3,[r10,#tri_p1+R3_y]\n\t"
		"ldrh	r4,[r10,#tri_p2+R3_x]\n\t"
		"ldrh	r5,[r10,#tri_p2+R3_y]\n\t"
		"bl	init_pb\n\t"
		/* skip tris of negative or zero area */
		"ble	7f\n\t"

		"ldrsh	r2,[r10,#tri_p1+R3_x]\n\t"
		"ldrsh	r3,[r10,#tri_p1+R3_y]\n\t"
		"ldrsh	r4,[r10,#tri_p2+R3_x]\n\t"
		"ldrsh	r5,[r10,#tri_p2+R3_y]\n\t"

		/* compute tri AABB */
		/* ascending sort in x-direction */
		"movs	r6,r0\n\t"
		"movs	r7,r2\n\t"
		"movs	r8,r4\n\t"

		"cmp	r6,r7\n\t"
		"ble	31f\n\t"

		"movs	r6,r2\n\t"
		"movs	r7,r0\n\t"
	"31:\n\t"
		"cmp	r6,r8\n\t"
		"ble	32f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"32:\n\t"
		"cmp	r7,r8\n\t"
		"ble	33f\n\t"

		"movs	r8,r7\n\t"
	"33:\n\t"
		/* intersect tri x-span with fb x-span */
		"cmp	r6,#0\n\t"
		"bge	331f\n\t"
		"movs	r6,#0\n\t"
	"331:\n\t"
		"cmp	r8,#320\n\t"
		"blt	332f\n\t"
		"movw	r8,#320-1\n\t"
	"332:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"

		/* store x_min, preload x_min and x_max */
		"str	r6,[r12,#-4]\n\t"
		"movs	r11,r6\n\t"
		"movs	r14,r8\n\t"

		/* ascending sort in y-direction */
		"movs	r6,r1\n\t"
		"movs	r7,r3\n\t"
		"movs	r8,r5\n\t"

		"cmp	r6,r7\n\t"
		"ble	34f\n\t"

		"movs	r6,r3\n\t"
		"movs	r7,r1\n\t"
	"34:\n\t"
		"cmp	r6,r8\n\t"
		"ble	35f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"35:\n\t"
		"cmp	r7,r8\n\t"
		"ble	36f\n\t"

		"movs	r8,r7\n\t"
	"36:\n\t"
		/* intersect tri y-span with fb y-span */
		"cmp	r6,#0\n\t"
		"bge	361f\n\t"
		"movs	r6,#0\n\t"
	"361:\n\t"
		"cmp	r8,#240\n\t"
		"blt	362f\n\t"
		"movs	r8,#240-1\n\t"
	"362:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"

		/* store y_max, preload y_min */
		"str	r8,[r12,#-8]\n\t"
		"movs	r7,r6\n\t"

		/* preload p0 */
		"movs	r2,r0\n\t"
		"movs	r3,r1\n\t"
		/* preload pb.e01, pb.e02, pb.area */
		"ldm	r12,{r4-r6}\n\t"

		/* preload color and fb ptr */
		"ldr	r8,[sp]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"mvns	r8,r8\n\t"

		"stmdb	sp!,{r9-r10}\n\t" /* att: room for pb_size bytes */

		/* iterate AABB along y */
		"movs	r9,#640\n\t"
		"mla	%[fb],r7,r9,%[fb]\n\t"
	"4:\n\t"
		/* iterate AABB along x */
	"5:\n\t"
		/* get barycentric coords for x,y */
		/* see barycentric.s:get_coord */
		"subs	r0,r11,r2\n\t"
		"subs	r1,r7,r3\n\t"

		"movs	r9,r0\n\t"
		"movs	r10,r1\n\t"

		"smulbt	r0,r0,r5\n\t"
		"smulbb	r1,r1,r4\n\t"
		"smulbt	r9,r9,r4\n\t"
		"smulbb	r10,r10,r5\n\t"

		/* if {s|t} < 0 || (s+t) > pb.area then pixel is outside */
		"subs	r1,r1,r9\n\t"
		"blt	6f\n\t"
		"subs	r0,r0,r10\n\t"
		"blt	6f\n\t"

		"adds	r0,r0,r1\n\t"
		"cmp	r0,r6\n\t"
		"bgt	6f\n\t"
		/* plot pixel */
		"strh	r8,[%[fb],r11,lsl #1]\n\t"
	"6:\n\t"
		"adds	r11,r11,#1\n\t"
		"cmp	r11,r14\n\t"
		"bls	5b\n\t"

		"ldrd	r1,r11,[sp,#-8-pb_size+8]\n\t"
		"adds	%[fb],%[fb],#640\n\t"
		"adds	r7,r7,#1\n\t"
		"cmp	r7,r1\n\t"
		"bls	4b\n\t"

		"ldmia	sp!,{r9-r10}\n\t"
	"7:\n\t"
		"adds	r10,r10,#tri_size\n\t"
		"cmp	r10,r9\n\t"
		"bne	3b\n\t"

		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_rot3d_solid_trilist_bc_zb(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse backface-culled z-buffered solid trilist rotating CW */
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"stmdb	sp!,{%[idx],%[fb]}\n\t"
		"movs	r0,#0x80008000\n\t"
		"movs	r1,#0x80008000\n\t"
		"movs	r2,#0x80008000\n\t"
		"movs	r3,#0x80008000\n\t"
		"movs	r4,#0x80008000\n\t"
		"movs	r5,#0x80008000\n\t"
		"movs	r6,#0x80008000\n\t"
		"movs	r7,#0x80008000\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R3 */
	".equ	R3_x, R_size * 0\n\t" /* short */
	".equ	R3_y, R_size * 1\n\t" /* short */
	".equ	R3_z, R_size * 2\n\t" /* short */
	".equ	R3_size, R_size * 3\n\t"

	/* struct tri */
	".equ	tri_p0, R3_size * 0\n\t" /* R3 */
	".equ	tri_p1, R3_size * 1\n\t" /* R3 */
	".equ	tri_p2, R3_size * 2\n\t" /* R3 */
	".equ	tri_size, R3_size * 3\n\t"

	/* matrix element, aka E */
	".equ	E_size, 4\n\t" /* word */

	/* struct row */
	".equ	row_x, E_size * 0\n\t" /* word */
	".equ	row_y, E_size * 1\n\t" /* word */
	".equ	row_z, E_size * 2\n\t" /* word */
	".equ	row_size, E_size * 3\n\t"

	/* struct mat */
	".equ	mat_0, row_size * 0\n\t" /* row */
	".equ	mat_1, row_size * 1\n\t" /* row */
	".equ	mat_2, row_size * 2\n\t" /* row */
	".equ	mat_size, row_size * 3\n\t"

	/* struct pb (parallelogram basis) */
	".equ	pb_e01,  R2_size * 0\n\t" /* R2 */
	".equ	pb_e02,  R2_size * 1\n\t" /* R2 */
	".equ	pb_area, R2_size * 2\n\t" /* word */
	".equ	pb_at0, R2_size * 2 + 4 + R_size * 0\n\t" /* short */
	".equ	pb_at1, R2_size * 2 + 4 + R_size * 1\n\t" /* short */
	".equ	pb_at2, R2_size * 2 + 4 + R_size * 2\n\t" /* short */
	".equ	pb_at3, R2_size * 2 + 4 + R_size * 3\n\t" /* short */
	".equ	pb_size, R2_size * 2 + 4 + R_size * 4\n\t"

	/* plot tris */
	/* transform obj -> scr */
		"movs	r0,#0x30\n\t" /* r10: cosB */
		"bl	cos15\n\t"
		"movs	r10,r0\n\t"
		"movs	r0,#0x30\n\t" /* r9: sinB */
		"bl	sin15\n\t"
		"movs	r9,r0\n\t"

		"movs	r0,%[idx]\n\t" /* r1: cosA */
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		"movs	r0,%[idx]\n\t" /* r0: sinA */
		"bl	sin15\n\t"

		/* preload mat_{0,1,2} */
		"movs	r3,r1\n\t"	/* m00: cosA */
		"mul	r4,r0,r10\n\t"	/* m01: sinA cosB */
		"mul	r5,r0,r9\n\t"	/* m02: sinA sinB */

		"negs	r6,r0\n\t"	/* m10: -sinA */
		"mul	r7,r1,r10\n\t"	/* m11: cosA cosB */
		"mul	r8,r1,r9\n\t"	/* m12: cosA sinB */

		"movs	r11,r10\n\t"	/* m22: cosB */
		"negs	r10,r9\n\t"	/* m21: -sinB */
		"movs	r9,#0\n\t"	/* m20: 0 */

		/* fx2.30 -> fx17.15 */
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#0\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"
		"asrs	r7,r7,#15\n\t"
		"adcs	r7,r7,#0\n\t"
		"asrs	r8,r8,#15\n\t"
		"adcs	r8,r8,#0\n\t"

		/* store mat_0 */
		"stmdb	sp!,{r3-r5}\n\t"

		"ldr	r12,=mesh_obj\n\t"
		"ldr	r12,[r12]\n\t"
		"ldrh	r14,[r12],#2\n\t"
		"movs	r0,#tri_size\n\t"
		"mul	r14,r14,r0\n\t"
		"adds	r14,r14,r12\n\t"

		/* store mesh_obj_end */
		"stmdb	sp!,{r14}\n\t"
		"ldr	r14,=mesh_scr\n\t"
	"2:\n\t"
		"ldrsh	r0,[r12],#2\n\t" /* v_in.x */
		"ldrsh	r1,[r12],#2\n\t" /* v_in.y */
		"ldrsh	r2,[r12],#2\n\t" /* v_in.z */

		/* inline mul_vec3_mat to reduce reg pressure */
		"muls	r3,r3,r0\n\t"
		"muls	r4,r4,r0\n\t"
		"muls	r5,r5,r0\n\t"

		"mla	r3,r6,r1,r3\n\t"
		"mla	r4,r7,r1,r4\n\t"
		"mla	r5,r8,r1,r5\n\t"

		"mla	r3,r9, r2,r3\n\t"
		"mla	r4,r10,r2,r4\n\t"
		"mla	r5,r11,r2,r5\n\t"

		/* fx16.15 -> int16 */
		"asrs	r3,r3,#15\n\t"
		"adcs	r3,r3,#160\n\t"
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#120\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"

		"strh	r3,[r14],#2\n\t" /* v_out.x */
		"strh	r4,[r14],#2\n\t" /* v_out.y */
		"strh	r5,[r14],#2\n\t" /* v_out.z */

		/* restore mesh_obj_end and mat_0 */
		"ldm	sp,{r0,r3-r5}\n\t"

		"cmp	r0,r12\n\t"
		"bne	2b\n\t"

		"adds	sp,sp,#4 * 4\n\t"

		/* scan-convert the scr-space tris */
		"ldr	r10,=mesh_scr\n\t"
		"movs	r9,r14\n\t"
		"subs	sp,sp,#8+pb_size\n\t"
	"3:\n\t"
		"adds	r12,sp,#8\n\t"
		/* compute parallelogram basis */
		"ldrsh	r0,[r10,#tri_p0+R3_x]\n\t"
		"ldrsh	r1,[r10,#tri_p0+R3_y]\n\t"
		"ldrh	r2,[r10,#tri_p1+R3_x]\n\t"
		"ldrh	r3,[r10,#tri_p1+R3_y]\n\t"
		"ldrh	r4,[r10,#tri_p2+R3_x]\n\t"
		"ldrh	r5,[r10,#tri_p2+R3_y]\n\t"
		"bl	init_pb\n\t"
		/* skip tris of negative or zero area */
		"ble	7f\n\t"

		"ldrh	r6,[r10,#tri_p0+R3_z]\n\t"
		"ldrsh	r2,[r10,#tri_p1+R3_x]\n\t"
		"ldrsh	r3,[r10,#tri_p1+R3_y]\n\t"
		"ldrh	r7,[r10,#tri_p1+R3_z]\n\t"
		"ldrsh	r4,[r10,#tri_p2+R3_x]\n\t"
		"ldrsh	r5,[r10,#tri_p2+R3_y]\n\t"
		"ldrh	r8,[r10,#tri_p2+R3_z]\n\t"

		/* stash tri attribs */
		"strh	r6,[sp,#8+pb_at0]\n\t"
		"strh	r7,[sp,#8+pb_at1]\n\t"
		"strh	r8,[sp,#8+pb_at2]\n\t"

		/* compute tri AABB */
		/* ascending sort in x-direction */
		"movs	r6,r0\n\t"
		"movs	r7,r2\n\t"
		"movs	r8,r4\n\t"

		"cmp	r6,r7\n\t"
		"ble	31f\n\t"

		"movs	r6,r2\n\t"
		"movs	r7,r0\n\t"
	"31:\n\t"
		"cmp	r6,r8\n\t"
		"ble	32f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"32:\n\t"
		"cmp	r7,r8\n\t"
		"ble	33f\n\t"

		"movs	r8,r7\n\t"
	"33:\n\t"
		/* intersect tri x-span with fb x-span */
		"cmp	r6,#0\n\t"
		"bge	331f\n\t"
		"movs	r6,#0\n\t"
	"331:\n\t"
		"cmp	r8,#320\n\t"
		"blt	332f\n\t"
		"movw	r8,#320-1\n\t"
	"332:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"

		/* store x_min, preload x_min and x_max */
		"str	r6,[sp,#4]\n\t"
		"movs	r11,r6\n\t"
		"movs	r14,r8\n\t"

		/* ascending sort in y-direction */
		"movs	r6,r1\n\t"
		"movs	r7,r3\n\t"
		"movs	r8,r5\n\t"

		"cmp	r6,r7\n\t"
		"ble	34f\n\t"

		"movs	r6,r3\n\t"
		"movs	r7,r1\n\t"
	"34:\n\t"
		"cmp	r6,r8\n\t"
		"ble	35f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"35:\n\t"
		"cmp	r7,r8\n\t"
		"ble	36f\n\t"

		"movs	r8,r7\n\t"
	"36:\n\t"
		/* intersect tri y-span with fb y-span */
		"cmp	r6,#0\n\t"
		"bge	361f\n\t"
		"movs	r6,#0\n\t"
	"361:\n\t"
		"cmp	r8,#240\n\t"
		"blt	362f\n\t"
		"movs	r8,#240-1\n\t"
	"362:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"

		/* store y_max, preload y_min */
		"str	r8,[sp,#0]\n\t"
		"movs	r7,r6\n\t"

		/* preload p0 */
		"movs	r2,r0\n\t"
		"movs	r3,r1\n\t"
		/* preload pb.e01, pb.e02, pb.area */
		"ldm	r12,{r4-r6}\n\t"

		/* preload fb ptr */
		"ldr	%[fb],[sp,#8+pb_size+4]\n\t"

		"stmdb	sp!,{r9-r10}\n\t"

		/* iterate AABB along y */
		"movs	r9,#640\n\t"
		"mla	%[fb],r7,r9,%[fb]\n\t"
	"4:\n\t"
		/* iterate AABB along x */
	"5:\n\t"
		/* get barycentric coords for x,y */
		/* see barycentric.s:get_coord */
		"subs	r0,r11,r2\n\t"
		"subs	r1,r7,r3\n\t"

		"movs	r8,r0\n\t"
		"movs	r9,r1\n\t"

		"smulbt	r0,r0,r5\n\t"
		"smulbb	r1,r1,r4\n\t"
		"smulbt	r8,r8,r4\n\t"
		"smulbb	r9,r9,r5\n\t"

		/* if {s|t} < 0 || (s+t) > pb.area then pixel is outside */
		"subs	r1,r1,r8\n\t"
		"blt	6f\n\t"
		"subs	r0,r0,r9\n\t"
		"blt	6f\n\t"

		"adds	r8,r0,r1\n\t"
		"subs	r8,r6\n\t"
		"bgt	6f\n\t"

		"ldr	r10,[sp,#16+pb_at0]\n\t"
		"ldrh	r9,[sp,#16+pb_at2]\n\t"
		"negs	r8,r8\n\t"
		/* barycentric {s,t,u} in {r0,r1,r8} */

		/* att: for the next weighing we ideally want
		   a 32b * 16b -> 32b mul, alas smulwx is not it;
		   fall back to smulxy and cross fingers primitive
		   doubled surface area does not exceed 16b */
		"smulbb	r8,r8,r10\n\t"
		"smulbt	r0,r0,r10\n\t"
		"smulbb	r1,r1,r9\n\t"
		"adds	r8,r8,r0\n\t"
		"adds	r8,r8,r1\n\t"
		"ldrsh	r9,[%[fb],r11,lsl #1]\n\t"
		"sdiv	r8,r8,r6\n\t"
		/* update zbuf */
		"cmp	r8,r9\n\t"
		"blt	6f\n\t"
		"strh	r8,[%[fb],r11,lsl #1]\n\t"
	"6:\n\t"
		"adds	r11,r11,#1\n\t"
		"cmp	r11,r14\n\t"
		"bls	5b\n\t"

		"ldrd	r1,r11,[sp,#8]\n\t"
		"adds	%[fb],%[fb],#640\n\t"
		"adds	r7,r7,#1\n\t"
		"cmp	r7,r1\n\t"
		"bls	4b\n\t"

		"ldmia	sp!,{r9-r10}\n\t"
	"7:\n\t"
		"adds	r10,r10,#tri_size\n\t"
		"cmp	r10,r9\n\t"
		"bne	3b\n\t"

		"adds	sp,sp,#8+pb_size\n\t"
		"ldmia	sp!,{%[idx],%[fb]}\n\t"
	: /* none */
	: [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

static void alt_bounce_sphere(const uint16_t color, uint32_t ii, void *framebuffer)
{
	/* Inverse backface-culled solid indexed trilist bouncing and rotating on color bg */
	register uint16_t val_color asm ("r0") = color;
	register uint32_t val_i asm ("r11") = ii;
	register void *ptr_fb asm ("r12") = framebuffer;

	__asm__ __volatile__ (
	/* clear fb to solid color */
		"bfi	%[color],%[color],#16,#16\n\t"
		"stmdb	sp!,{%[color],%[idx],%[fb]}\n\t"
		"movs	r1,%[color]\n\t"
		"movs	r2,%[color]\n\t"
		"movs	r3,%[color]\n\t"
		"movs	r4,%[color]\n\t"
		"movs	r5,%[color]\n\t"
		"movs	r6,%[color]\n\t"
		"movs	r7,%[color]\n\t"
		"movs	r8,#(320*240*2/(8*4*4))\n\t"
	"1:\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"stm	%[fb]!,{r0-r7}\n\t"
		"subs	r8,r8,#1\n\t"
		"bne	1b\n\t"

	/* vertex coord component, aka R */
	".equ	R_size, 2\n\t" /* short */

	/* struct R3 */
	".equ	R3_x, R_size * 0\n\t" /* short */
	".equ	R3_y, R_size * 1\n\t" /* short */
	".equ	R3_z, R_size * 2\n\t" /* short */
	".equ	R3_size, R_size * 3\n\t"

	/* tri index */
	".equ	idx_size, 2\n\t" /* short */

	/* struct tri */
	".equ	tri_i0, idx_size * 0\n\t" /* short */
	".equ	tri_i1, idx_size * 1\n\t" /* short */
	".equ	tri_i2, idx_size * 2\n\t" /* short */
	".equ	tri_size, idx_size * 3\n\t"

	/* matrix element, aka E */
	".equ	E_size, 4\n\t" /* word */

	/* struct row */
	".equ	row_x, E_size * 0\n\t" /* word */
	".equ	row_y, E_size * 1\n\t" /* word */
	".equ	row_z, E_size * 2\n\t" /* word */
	".equ	row_size, E_size * 3\n\t"

	/* struct mat */
	".equ	mat_0, row_size * 0\n\t" /* row */
	".equ	mat_1, row_size * 1\n\t" /* row */
	".equ	mat_2, row_size * 2\n\t" /* row */
	".equ	mat_size, row_size * 3\n\t"

	/* struct pb (parallelogram basis) */
	".equ	pb_e01,  R2_size * 0\n\t" /* R2 */
	".equ	pb_e02,  R2_size * 1\n\t" /* R2 */
	".equ	pb_area, R2_size * 2\n\t" /* word */
	".equ	pb_size, R2_size * 2 + 4\n\t"

	/* plot tris */
	/* transform obj -> scr */
		"negs	r0,%[idx]\n\t" /* r10: cosB */
		"bl	cos15\n\t"
		"movs	r10,r0\n\t"
		"negs	r0,%[idx]\n\t" /* r9: sinB */
		"bl	sin15\n\t"
		"movs	r9,r0\n\t"

		"movs	r0,%[idx]\n\t" /* r1: cosA */
		"bl	cos15\n\t"
		"movs	r1,r0\n\t"
		"movs	r0,%[idx]\n\t" /* r0: sinA */
		"bl	sin15\n\t"

		/* preload mat_{0,1,2} */
		"movs	r3,r1\n\t"	/* m00: cosA */
		"mul	r4,r0,r10\n\t"	/* m01: sinA cosB */
		"mul	r5,r0,r9\n\t"	/* m02: sinA sinB */

		"negs	r6,r0\n\t"	/* m10: -sinA */
		"mul	r7,r1,r10\n\t"	/* m11: cosA cosB */
		"mul	r8,r1,r9\n\t"	/* m12: cosA sinB */

		"movs	r11,r10\n\t"	/* m22: cosB */
		"negs	r10,r9\n\t"	/* m21: -sinB */
		"movs	r9,#0\n\t"	/* m20: 0 */

		/* store translation (0, -64 * abs(cosA), 0) */
		"asrs	r0,r1,#31\n\t"
		"eors	r1,r1,r0\n\t"
		"subs	r1,r1,r0\n\t"
		"lsls	r0,r1,#6\n\t"
		"negs	r0,r0\n\t"
		"movs	r1,#0\n\t"
		"stmdb	sp!,{r0-r1}\n\t"
		"stmdb	sp!,{r1}\n\t"

		/* fx2.30 -> fx17.15 */
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#0\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"
		"asrs	r7,r7,#15\n\t"
		"adcs	r7,r7,#0\n\t"
		"asrs	r8,r8,#15\n\t"
		"adcs	r8,r8,#0\n\t"

		/* store mat_0 */
		"stmdb	sp!,{r3-r5}\n\t"

		"ldr	r12,=sphere_obj\n\t"
		"ldrh	r14,[r12],#2\n\t"
		"movs	r0,#R3_size\n\t"
		"mul	r14,r14,r0\n\t"
		"adds	r14,r14,r12\n\t"

		/* store mesh_obj_end */
		"stmdb	sp!,{r14}\n\t"
		"ldr	r14,=mesh_scr\n\t"
	"2:\n\t"
		"ldrsh	r0,[r12],#2\n\t" /* v_in.x */
		"ldrsh	r1,[r12],#2\n\t" /* v_in.y */
		"ldrsh	r2,[r12],#2\n\t" /* v_in.z */

		/* inline mul_vec3_mat_tr to reduce reg pressure */
		"muls	r3,r3,r0\n\t"
		"muls	r4,r4,r0\n\t"
		"muls	r5,r5,r0\n\t"

		"ldr	r0,[sp,#16]\n\t"

		"mla	r3,r6,r1,r3\n\t"
		"mla	r4,r7,r1,r4\n\t"
		"mla	r5,r8,r1,r5\n\t"

		"ldr	r1,[sp,#20]\n\t"

		"mla	r3,r9, r2,r3\n\t"
		"mla	r4,r10,r2,r4\n\t"
		"mla	r5,r11,r2,r5\n\t"

		"ldr	r2,[sp,#24]\n\t"

		"adds	r3,r3,r0\n\t"
		"adds	r4,r4,r1\n\t"
		"adds	r5,r5,r2\n\t"

		/* fx16.15 -> int16 */
		"asrs	r3,r3,#15\n\t"
		"adcs	r3,r3,#160\n\t"
		"asrs	r4,r4,#15\n\t"
		"adcs	r4,r4,#150\n\t"
		"asrs	r5,r5,#15\n\t"
		"adcs	r5,r5,#0\n\t"

		"strh	r3,[r14],#2\n\t" /* v_out.x */
		"strh	r4,[r14],#2\n\t" /* v_out.y */
		"strh	r5,[r14],#2\n\t" /* v_out.z */

		/* restore mesh_obj_end and mat_0 */
		"ldm	sp,{r0,r3-r5}\n\t"

		"cmp	r0,r12\n\t"
		"bne	2b\n\t"

		"adds	sp,sp,#7 * 4\n\t"

		/* scan-convert the scr-space tris */
		"ldr	r10,=sphere_idx\n\t"
		"ldrh	r9,[r10],#2\n\t"
		"movs	r14,#tri_size\n\t"
		"mul	r9,r9,r14\n\t"
		"adds	r9,r9,r10\n\t"
	"3:\n\t"
		"ldr	r14,=mesh_scr\n\t"
		"ldrh	r1,[r10],#idx_size\n\t"
		"ldrh	r3,[r10],#idx_size\n\t"
		"ldrh	r5,[r10],#idx_size\n\t"

		"adds	r1,r1,r1,lsl #1\n\t"
		"adds	r3,r3,r3,lsl #1\n\t"
		"adds	r5,r5,r5,lsl #1\n\t"
		"adds	r1,r14,r1,lsl #1\n\t"
		"adds	r3,r14,r3,lsl #1\n\t"
		"adds	r5,r14,r5,lsl #1\n\t"

		"subs	r12,sp,#pb_size\n\t"
		/* compute parallelogram basis */
		"ldrsh	r0,[r1,#R3_x]\n\t"
		"ldrsh	r1,[r1,#R3_y]\n\t"
		"ldrsh	r2,[r3,#R3_x]\n\t"
		"ldrsh	r3,[r3,#R3_y]\n\t"
		"ldrsh	r4,[r5,#R3_x]\n\t"
		"ldrsh	r5,[r5,#R3_y]\n\t"
		/* preserve p1-p2 from clobber by init_pb */
		"movs	r6,r2\n\t"
		"movs	r7,r3\n\t"
		"movs	r8,r4\n\t"
		"movs	r11,r5\n\t"
		"bl	init_pb\n\t"
		/* skip tris of negative or zero area */
		"ble	7f\n\t"

		/* restore p1-p2 */
		"movs	r2,r6\n\t"
		"movs	r3,r7\n\t"
		"movs	r4,r8\n\t"
		"movs	r5,r11\n\t"

		/* compute tri AABB */
		/* ascending sort in x-direction */
		"movs	r6,r0\n\t"
		"movs	r7,r2\n\t"
		"movs	r8,r4\n\t"

		"cmp	r6,r7\n\t"
		"ble	31f\n\t"

		"movs	r6,r2\n\t"
		"movs	r7,r0\n\t"
	"31:\n\t"
		"cmp	r6,r8\n\t"
		"ble	32f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"32:\n\t"
		"cmp	r7,r8\n\t"
		"ble	33f\n\t"

		"movs	r8,r7\n\t"
	"33:\n\t"
		/* intersect tri x-span with fb x-span */
		"cmp	r6,#0\n\t"
		"bge	331f\n\t"
		"movs	r6,#0\n\t"
	"331:\n\t"
		"cmp	r8,#320\n\t"
		"blt	332f\n\t"
		"movw	r8,#320-1\n\t"
	"332:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"

		/* store x_min, preload x_min and x_max */
		"str	r6,[r12,#-4]\n\t"
		"movs	r11,r6\n\t"
		"movs	r14,r8\n\t"

		/* ascending sort in y-direction */
		"movs	r6,r1\n\t"
		"movs	r7,r3\n\t"
		"movs	r8,r5\n\t"

		"cmp	r6,r7\n\t"
		"ble	34f\n\t"

		"movs	r6,r3\n\t"
		"movs	r7,r1\n\t"
	"34:\n\t"
		"cmp	r6,r8\n\t"
		"ble	35f\n\t"

		"bfi	r6,r8,#16,#16\n\t"
		"sxth	r8,r6\n\t"
		"asrs	r6,r6,#16\n\t"
	"35:\n\t"
		"cmp	r7,r8\n\t"
		"ble	36f\n\t"

		"movs	r8,r7\n\t"
	"36:\n\t"
		/* intersect tri y-span with fb y-span */
		"cmp	r6,#0\n\t"
		"bge	361f\n\t"
		"movs	r6,#0\n\t"
	"361:\n\t"
		"cmp	r8,#240\n\t"
		"blt	362f\n\t"
		"movs	r8,#240-1\n\t"
	"362:\n\t"
		"cmp	r6,r8\n\t"
		"bgt	7f\n\t"

		/* store y_max, preload y_min */
		"str	r8,[r12,#-8]\n\t"
		"movs	r7,r6\n\t"

		/* preload p0 */
		"movs	r2,r0\n\t"
		"movs	r3,r1\n\t"
		/* preload pb.e01, pb.e02, pb.area */
		"ldm	r12,{r4-r6}\n\t"

		/* preload color and fb ptr */
		"ldr	r8,[sp]\n\t"
		"ldr	%[fb],[sp,#8]\n\t"
		"mvns	r8,r8\n\t"

		"stmdb	sp!,{r9-r10}\n\t" /* att: room for pb_size bytes */

		/* iterate AABB along y */
		"movs	r9,#640\n\t"
		"mla	%[fb],r7,r9,%[fb]\n\t"
	"4:\n\t"
		/* iterate AABB along x */
	"5:\n\t"
		/* get barycentric coords for x,y */
		/* see barycentric.s:get_coord */
		"subs	r0,r11,r2\n\t"
		"subs	r1,r7,r3\n\t"

		"movs	r9,r0\n\t"
		"movs	r10,r1\n\t"

		"smulbt	r0,r0,r5\n\t"
		"smulbb	r1,r1,r4\n\t"
		"smulbt	r9,r9,r4\n\t"
		"smulbb	r10,r10,r5\n\t"

		/* if {s|t} < 0 || (s+t) > pb.area then pixel is outside */
		"subs	r1,r1,r9\n\t"
		"blt	6f\n\t"
		"subs	r0,r0,r10\n\t"
		"blt	6f\n\t"

		"adds	r0,r0,r1\n\t"
		"cmp	r0,r6\n\t"
		"bgt	6f\n\t"
		/* plot pixel */
		"strh	r8,[%[fb],r11,lsl #1]\n\t"
	"6:\n\t"
		"adds	r11,r11,#1\n\t"
		"cmp	r11,r14\n\t"
		"bls	5b\n\t"

		"ldrd	r1,r11,[sp,#-8-pb_size+8]\n\t"
		"adds	%[fb],%[fb],#640\n\t"
		"adds	r7,r7,#1\n\t"
		"cmp	r7,r1\n\t"
		"bls	4b\n\t"

		"ldmia	sp!,{r9-r10}\n\t"
	"7:\n\t"
		"cmp	r10,r9\n\t"
		"bne	3b\n\t"

		"ldmia	sp!,{%[color],%[idx],%[fb]}\n\t"
	: /* none */
	: [color] "r" (val_color),
	  [idx] "r" (val_i),
	  [fb] "r" (ptr_fb)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
	  "r9", "r10", "r14", "cc");
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Disable wakeup source PIN1 */
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1_LOW);

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_LTDC_Init();
	MX_SPI2_Init();
	MX_OCTOSPI1_Init();
	MX_SAI1_Init();

	/* Initialize interrupts */
	MX_NVIC_Init();
	/* USER CODE BEGIN 2 */

	/* Enable caching */
	SCB_EnableICache();
#if 0
	/* att: Don't enable d-caches; framebuffer is not excluded from caching
	   resulting in artifacts, and we don't do many non-local reads */
	SCB_EnableDCache();

#endif
	lcd_init(&hspi2);
	memset(framebuffer, 0, sizeof(framebuffer));

	HAL_LTDC_SetAddress(&hltdc, (uint32_t) &framebuffer[1], LTDC_LAYER_1);

	/* USER CODE END 2 */

	flash_memory_map(&hospi1);

	/* Sanity check, sometimes this is triggered */
	uint32_t add = 0x90000000;
	uint32_t* ptr = (uint32_t*)add;
	if(*ptr == 0x88888888) {
		Error_Handler();
	}

	/* Prepare procedural assets */
	createIndexedPolarSphere(
		(count_arr_t *) &sphere_obj,
		(count_idx_t *) &sphere_idx,
		9, 17, 80.f);

	/* Enable DWT cycle counter */
	CoreDebug->DEMCR = CoreDebug->DEMCR | 0x01000000; /* enable trace */
	DWT->LAR = 0xC5ACCE55;		/* unlock access to DWT registers */
	DWT->CYCCNT = 0;		/* clear DWT cycle counter */
	DWT->CTRL = DWT->CTRL | 1;	/* enable DWT cycle counter */

	uint8_t alt = 0;
	uint16_t color = 0;
	uint32_t mask = 0x1f, shift = 0;
	uint32_t i = 0, ii = 0, di = 1, si = 1, last_press = 0;
	uint32_t mi = 0;

	while (1) {
		const uint32_t buttons = buttons_get();
		const uint32_t press_lim = 8;
		if (buttons & B_POWER) {
			deepsleep();
		}
		if (buttons & B_Left) {
			/* red */
			mask = 0x1f;
			shift = 5 + 6 + 0;
			color = mask << shift;
		}
		if (buttons & B_Right) {
			/* green */
			mask = 0x3f;
			shift = 5 + 0 + 0;
			color = mask << shift;
		}
		if (buttons & B_Up) {
			/* blue */
			mask = 0x1f;
			shift = 0 + 0 + 0;
			color = mask << shift;
		}
		if (buttons & B_Down) {
			color = 0;
		}
		if (buttons & B_A && alt < ALT_NUM - 1 && i - last_press > press_lim) {
			last_press = i;
			alt++;
		}
		if (buttons & B_B && alt > 0 && i - last_press > press_lim) {
			last_press = i;
			alt--;
		}
		if (buttons & B_PAUSE && i - last_press > press_lim) {
			last_press = i;
			di = di ? 0 : si;
		}
		if (buttons & B_TIME && i - last_press > press_lim) {
			last_press = i;
			si <<= 1;
			if (si == 8)
				si = 1;
			if (di)
				di = si;
		}
		if (buttons & B_GAME && i - last_press > press_lim) {
			last_press = i;
			mi++;
			if (mi == sizeof(mseq) / sizeof(mseq[0]))
				mi = 0;
			mesh_obj = mseq[mi];
		}

		const uint32_t st = DWT->CYCCNT;

		/* What scene are we producing a frame from? */
		if (alt == 0) {
			/* Checkers of selected color */
			int32_t off = 64 * sin15(ii * 4);
			__asm__ __volatile__ (
				"asrs	%[arg],%[arg],#15\n\t"
				"adcs	%[arg],%[arg],#0\n\t"
			: [arg] "=r" (off)
			: "0" (off)
			: "cc");

			for(int y=0, row=0; y < 240; y++, row+=320) {
				for(int x=0; x < 320; x++) {
					if(((x + off) & 32) ^ (((y + off) & 32))) {
						framebuffer[i & 1][row + x] = color;
					} else {
						framebuffer[i & 1][row + x] = 0xffff;
					}
				}
			}
		}
		else if (alt == 1) {
			/* XOR pattern of selected color */
			int32_t off_x = 64 * sin15(ii * 4);
			int32_t off_y = 128 * cos15(ii * 2);
			__asm__ __volatile__ (
				"asrs	%[arg_x],%[arg_x],#15\n\t"
				"adcs	%[arg_x],%[arg_x],#0\n\t"
				"asrs	%[arg_y],%[arg_y],#15\n\t"
				"adcs	%[arg_y],%[arg_y],#0\n\t"
			: [arg_x] "=r" (off_x),
			  [arg_y] "=r" (off_y)
			: "0" (off_x),
			  "1" (off_y)
			: "cc");

			for(int y=0, row=0; y < 240; y++, row+=320) {
				for(int x=0; x < 320; x++) {
					framebuffer[i & 1][row + x] = ((x + off_x ^ y + off_y) >> 2 & mask) << shift;
				}
			}
		}
		else if (alt == 2) {
			alt_rot2d_dots(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 3) {
			alt_rot2d_lines(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 4) {
			alt_rot2d_wire_triforce(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 5) {
			alt_rot2d_solid_triforce(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 6) {
			mask_shift[0] = mask;
			mask_shift[1] = shift;
			alt_rot2d_solid_tri(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 7) {
			alt_rot2d_wire_trilist_bc(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 8) {
			alt_rot2d_solid_trilist_bc(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 9) {
			alt_rot3d_wire_trilist(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 10) {
			alt_rot3d_wire_trilist_bc(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 11) {
			alt_rot3d_solid_trilist_bc(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 12) {
			alt_rot3d_solid_trilist_bc_zb(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 13) {
			alt_bounce_sphere(color, ii, framebuffer + (i & 1));
		}
		else if (alt == 14) {
			/* Color font 8x16 on black bg */
			register uint16_t val_color asm ("r0") = 0;
			register void *ptr_fb asm ("r12") = framebuffer + (i & 1);

			__asm__ __volatile__ (
			/* clear fb to solid color */
				"bfi	%[color],%[color],#16,#16\n\t"
				"stmdb	sp!,{%[color],%[fb]}\n\t"
				"movs	r1,%[color]\n\t"
				"movs	r2,%[color]\n\t"
				"movs	r3,%[color]\n\t"
				"movs	r4,%[color]\n\t"
				"movs	r5,%[color]\n\t"
				"movs	r6,%[color]\n\t"
				"movs	r7,%[color]\n\t"
				"movs	r8,#(320*240*2/(8*4*4))\n\t"
			"1:\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"subs	r8,r8,#1\n\t"
				"bne	1b\n\t"
				"ldmia	sp!,{%[color],%[fb]}\n\t"
			: /* none */
			: [color] "r" (val_color),
			  [fb] "r" (ptr_fb)
			: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "cc");

			for (int32_t y = 0, ch = ii; y < 240 / 16; y++)
				for (int32_t x = 0; x < 320 / 8; x++, ch++) {
					pixmap8x16(~color, framebuffer[i & 1] + 320 * 16 * y + 8 * x,
						&fnt_wang_8x16 + 16 * (ch & 0xff));
				}
		}
		else if (alt == 15) {
			/* Color font 8x8 on black bg */
			register uint16_t val_color asm ("r0") = 0;
			register void *ptr_fb asm ("r12") = framebuffer + (i & 1);

			__asm__ __volatile__ (
			/* clear fb to solid color */
				"bfi	%[color],%[color],#16,#16\n\t"
				"stmdb	sp!,{%[color],%[fb]}\n\t"
				"movs	r1,%[color]\n\t"
				"movs	r2,%[color]\n\t"
				"movs	r3,%[color]\n\t"
				"movs	r4,%[color]\n\t"
				"movs	r5,%[color]\n\t"
				"movs	r6,%[color]\n\t"
				"movs	r7,%[color]\n\t"
				"movs	r8,#(320*240*2/(8*4*4))\n\t"
			"1:\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"subs	r8,r8,#1\n\t"
				"bne	1b\n\t"
				"ldmia	sp!,{%[color],%[fb]}\n\t"
			: /* none */
			: [color] "r" (val_color),
			  [fb] "r" (ptr_fb)
			: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "cc");

			for (int32_t y = 0, ch = ii; y < 240 / 8; y++)
				for (int32_t x = 0; x < 320 / 8; x++, ch++) {
					pixmap8x8(~color, framebuffer[i & 1] + 320 * 8 * y + 8 * x,
						&fnt_wang_8x8 + 8 * (ch & 0xff));
				}
		}
		else if (alt == 16) {
			/* Blue turtle-shell pattern */
#if 1
			for(int y=0, row=0; y < 240; y++, row+=320) {
				int32_t off_y = 15 * cos16((y + ii * 4) * 4);
				__asm__ __volatile__ (
					"asrs	%[arg_y],%[arg_y],#16\n\t"
					"adcs	%[arg_y],%[arg_y],#16\n\t"
				: [arg_y] "=r" (off_y)
				: "0" (off_y)
				: "cc");

				for(int x=0; x < 320; x++) {
					int32_t off_x = 15 * sin16((x + ii * 4) * 4);
					__asm__ __volatile__ (
						"asrs	%[arg_x],%[arg_x],#16\n\t"
						"adcs	%[arg_x],%[arg_x],#16\n\t"
					: [arg_x] "=r" (off_x)
					: "0" (off_x)
					: "cc");

					framebuffer[i & 1][row+x] = off_x + off_y;
				}
			}
#else
			/* gcc 12.2 codegen emits a few excessive moves from the above */
			register uint32_t val_i_x16 asm ("r9") = ii * 4 * 4;
			register uint32_t val_y_lim asm ("r10") = (240 + ii * 4) * 4;
			register void *ptr_fb asm ("r11") = framebuffer + (i & 1);
			__asm__ __volatile__ (
				"stmdb	sp!,{%[fb]}\n\t"
				"ldr	r12,=cosLUT15_128 - 2\n\t"
				"movs	r4,%[i_x16]\n\t"
			"1:\n\t"
				"movs	r0,r4\n\t"
				"bl	cos\n\t"
				"rsbs	r1,r0,r0,lsl #4\n\t"

				"asrs	r1,r1,#15\n\t"
				"adcs	r1,r1,#16\n\t"

				"movs	r5,%[i_x16]\n\t"
				"adds	r3,%[i_x16],#320 * 4\n\t"
				".balign 32\n\t"
			"2:\n\t"
				"movs	r0,r5\n\t"
				"bl	sin\n\t"
				"rsbs	r2,r0,r0,lsl #4\n\t"

				"asrs	r2,r2,#15\n\t"
				"adcs	r2,r2,#16\n\t"

				"adds	r2,r2,r1\n\t"
				"adds	r5,r5,#4\n\t"
				"strh	r2,[%[fb]],#2\n\t"
				"cmp	r3,r5\n\t"
				"bne	2b\n\t"

				"adds	r4,r4,#4\n\t"
				"cmp	r4,%[y_lim]\n\t"
				"bne	1b\n\t"
				"ldmia	sp!,{%[fb]}"
			: /* none */
			: [i_x16] "r" (val_i_x16),
			  [y_lim] "r" (val_y_lim),
			  [fb] "r" (ptr_fb)
			: "r0", "r1", "r2", "r3", "r4", "r5", "r12", "r14", "cc");
#endif
		}
		else {
			/* vsynced counter */
			register uint16_t val_color asm ("r0") = 0;
			register void *ptr_fb asm ("r12") = framebuffer + (i & 1);

			__asm__ __volatile__ (
			/* clear fb to solid color */
				"bfi	%[color],%[color],#16,#16\n\t"
				"stmdb	sp!,{%[color],%[fb]}\n\t"
				"movs	r1,%[color]\n\t"
				"movs	r2,%[color]\n\t"
				"movs	r3,%[color]\n\t"
				"movs	r4,%[color]\n\t"
				"movs	r5,%[color]\n\t"
				"movs	r6,%[color]\n\t"
				"movs	r7,%[color]\n\t"
				"movs	r8,#(320*240*2/(8*4*4))\n\t"
			"1:\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"stm	%[fb]!,{r0-r7}\n\t"
				"subs	r8,r8,#1\n\t"
				"bne	1b\n\t"
				"ldmia	sp!,{%[color],%[fb]}\n\t"
			: /* none */
			: [color] "r" (val_color),
			  [fb] "r" (ptr_fb)
			: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "cc");

			void * const out = framebuffer[i & 1] + 320 * (240 - 16) / 2 + (320 - 8 * 8) / 2;
			print_x32(~color, out, ii);

			/* Print cycle counters */
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  0, cycle[0]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  1, cycle[1]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  2, cycle[2]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  3, cycle[3]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  4, cycle[4]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  5, cycle[5]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  6, cycle[6]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  7, cycle[7]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  8, cycle[8]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 *  9, cycle[9]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 * 10, cycle[10]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 * 11, cycle[11]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 * 12, cycle[12]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 * 13, cycle[13]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 * 14, cycle[14]);

			print_x32(~color, framebuffer[i & 1] + 320 * 16 * 0 + 9 * 8, cycle[15]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 * 1 + 9 * 8, cycle[16]);
			print_x32(~color, framebuffer[i & 1] + 320 * 16 * 2 + 9 * 8, cycle[17]);
		}

		const uint32_t dt = DWT->CYCCNT - st;
		const uint32_t old_cycle = cycle[alt];
		cycle[alt] = old_cycle ? (old_cycle + dt) / 2 : dt;

		/* Flip back and front framebuffers at vblank */
		HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t) &framebuffer[i & 1], LTDC_LAYER_1);
		HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);

		while ((hltdc.Instance->SRCR & LTDC_RELOAD_VERTICAL_BLANKING) != 0) {}

		i++;
		ii += di;
	}
}

static void deepsleep()
{
#ifndef VECT_TAB_SRAM
	/* Enable wakup by PIN1, the power button */
	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_LOW);

	lcd_backlight_off();

	HAL_PWR_EnterSTANDBYMode();

#endif
	HAL_NVIC_SystemReset();
}

/**
  * @brief Print 32-bit hexadecimal to a location in a fixed-geometry framebuffer
  * @param color: Foreground color to use
  * @param out_fb: Ptr in a fixed-geometry framebuffer
  * @param val: Value to print
  * @retval None
  */
static void print_x32(uint16_t color, void *out_fb, uint32_t val)
{
	for (uint32_t di = 0, ch = val; di < 8; di++, ch >>= 4) {
		void * const out = (uint16_t *)out_fb + 8 * 7 - 8 * di;
		if ((ch & 0xf) > 9)
			pixmap8x16(color, out, &fnt_wang_8x16 + 16 * ('A' - 10 + (ch & 0xf)));
		else
			pixmap8x16(color, out, &fnt_wang_8x16 + 16 * ('0' + (ch & 0xf)));
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	/** Supply configuration update enable
	*/
	HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
	/** Configure the main internal regulator output voltage
	*/
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

	while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
	/** Macro to configure the PLL clock source
	*/
	__HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSI);
	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 140;
	RCC_OscInitStruct.PLL.PLLP = 2;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLFRACN = 0;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
	                            |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
	                            |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
	
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SPI2
	                            |RCC_PERIPHCLK_SAI1|RCC_PERIPHCLK_OSPI
	                            |RCC_PERIPHCLK_CKPER;
	PeriphClkInitStruct.PLL2.PLL2M = 25;
	PeriphClkInitStruct.PLL2.PLL2N = 192;
	PeriphClkInitStruct.PLL2.PLL2P = 5;
	PeriphClkInitStruct.PLL2.PLL2Q = 2;
	PeriphClkInitStruct.PLL2.PLL2R = 5;
	PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
	PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
	PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
	PeriphClkInitStruct.PLL3.PLL3M = 4;
	PeriphClkInitStruct.PLL3.PLL3N = 9;
	PeriphClkInitStruct.PLL3.PLL3P = 2;
	PeriphClkInitStruct.PLL3.PLL3Q = 2;
	PeriphClkInitStruct.PLL3.PLL3R = 24;
	PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
	PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
	PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
	PeriphClkInitStruct.OspiClockSelection = RCC_OSPICLKSOURCE_CLKP;
	PeriphClkInitStruct.CkperClockSelection = RCC_CLKPSOURCE_HSI;
	PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
	PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_CLKP;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
	/* OCTOSPI1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(OCTOSPI1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(OCTOSPI1_IRQn);
}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{
	/* USER CODE BEGIN LTDC_Init 0 */

	/* USER CODE END LTDC_Init 0 */

	LTDC_LayerCfgTypeDef pLayerCfg = {0};
	LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

	/* USER CODE BEGIN LTDC_Init 1 */

	/* USER CODE END LTDC_Init 1 */
	hltdc.Instance = LTDC;
	hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
	hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
	hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
	hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IIPC;
	hltdc.Init.HorizontalSync = 9;
	hltdc.Init.VerticalSync = 1;
	hltdc.Init.AccumulatedHBP = 60;
	hltdc.Init.AccumulatedVBP = 7;
	hltdc.Init.AccumulatedActiveW = 380;
	hltdc.Init.AccumulatedActiveH = 247;
	hltdc.Init.TotalWidth = 392;
	hltdc.Init.TotalHeigh = 255;
	hltdc.Init.Backcolor.Blue = 0;
	hltdc.Init.Backcolor.Green = 0;
	hltdc.Init.Backcolor.Red = 0;
	if (HAL_LTDC_Init(&hltdc) != HAL_OK)
	{
		Error_Handler();
	}
	pLayerCfg.WindowX0 = 0;
	pLayerCfg.WindowX1 = 320;
	pLayerCfg.WindowY0 = 0;
	pLayerCfg.WindowY1 = 240;
	pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
	pLayerCfg.Alpha = 255;
	pLayerCfg.Alpha0 = 255;
	pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
	pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
	pLayerCfg.FBStartAdress = 0x24000000;
	pLayerCfg.ImageWidth = 320;
	pLayerCfg.ImageHeight = 240;
	pLayerCfg.Backcolor.Blue = 0;
	pLayerCfg.Backcolor.Green = 255;
	pLayerCfg.Backcolor.Red = 0;
	if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, LTDC_LAYER_1) != HAL_OK)
	{
		Error_Handler();
	}
	pLayerCfg1.WindowX0 = 0;
	pLayerCfg1.WindowX1 = 0;
	pLayerCfg1.WindowY0 = 0;
	pLayerCfg1.WindowY1 = 0;
	pLayerCfg1.Alpha = 0;
	pLayerCfg1.Alpha0 = 0;
	pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
	pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
	pLayerCfg1.FBStartAdress = GFXMMU_VIRTUAL_BUFFER0_BASE;
	pLayerCfg1.ImageWidth = 0;
	pLayerCfg1.ImageHeight = 0;
	pLayerCfg1.Backcolor.Blue = 0;
	pLayerCfg1.Backcolor.Green = 0;
	pLayerCfg1.Backcolor.Red = 0;
	if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, LTDC_LAYER_2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN LTDC_Init 2 */

	/* USER CODE END LTDC_Init 2 */
}

/**
  * @brief OCTOSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_OCTOSPI1_Init(void)
{
	/* USER CODE BEGIN OCTOSPI1_Init 0 */

	/* USER CODE END OCTOSPI1_Init 0 */

	OSPIM_CfgTypeDef sOspiManagerCfg = {0};

	/* USER CODE BEGIN OCTOSPI1_Init 1 */

	/* USER CODE END OCTOSPI1_Init 1 */
	/* OCTOSPI1 parameter configuration*/
	hospi1.Instance = OCTOSPI1;
	hospi1.Init.FifoThreshold = 4;
	hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
	hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
	hospi1.Init.DeviceSize = 20;
	hospi1.Init.ChipSelectHighTime = 2;
	hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
	hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
	hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
	hospi1.Init.ClockPrescaler = 1;
	hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
	hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
	hospi1.Init.ChipSelectBoundary = 0;
	hospi1.Init.ClkChipSelectHighTime = 0;
	hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
	hospi1.Init.MaxTran = 0;
	hospi1.Init.Refresh = 0;
	if (HAL_OSPI_Init(&hospi1) != HAL_OK)
	{
		Error_Handler();
	}
	sOspiManagerCfg.ClkPort = 1;
	sOspiManagerCfg.NCSPort = 1;
	sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
	if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN OCTOSPI1_Init 2 */

	/* USER CODE END OCTOSPI1_Init 2 */
}

/**
  * @brief SAI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI1_Init(void)
{
	/* USER CODE BEGIN SAI1_Init 0 */

	/* USER CODE END SAI1_Init 0 */

	/* USER CODE BEGIN SAI1_Init 1 */

	/* USER CODE END SAI1_Init 1 */
	hsai_BlockA1.Instance = SAI1_Block_A;
	hsai_BlockA1.Init.AudioMode = SAI_MODEMASTER_TX;
	hsai_BlockA1.Init.Synchro = SAI_ASYNCHRONOUS;
	hsai_BlockA1.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
	hsai_BlockA1.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
	hsai_BlockA1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
	hsai_BlockA1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
	hsai_BlockA1.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
	hsai_BlockA1.Init.MonoStereoMode = SAI_MONOMODE;
	hsai_BlockA1.Init.CompandingMode = SAI_NOCOMPANDING;
	hsai_BlockA1.Init.TriState = SAI_OUTPUT_NOTRELEASED;
	if (HAL_SAI_InitProtocol(&hsai_BlockA1, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SAI1_Init 2 */

	/* USER CODE END SAI1_Init 2 */
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{
	/* USER CODE BEGIN SPI2_Init 0 */

	/* USER CODE END SPI2_Init 0 */

	/* USER CODE BEGIN SPI2_Init 1 */

	/* USER CODE END SPI2_Init 1 */
	/* SPI2 parameter configuration*/
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 0x0;
	hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
	hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
	hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
	hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
	hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
	hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
	hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
	hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
	if (HAL_SPI_Init(&hspi2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI2_Init 2 */

	/* USER CODE END SPI2_Init 2 */
}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Stream0_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIO_Speaker_enable_GPIO_Port, GPIO_Speaker_enable_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8|GPIO_PIN_4, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_RESET);

	/*Configure GPIO pin : GPIO_Speaker_enable_Pin */
	GPIO_InitStruct.Pin = GPIO_Speaker_enable_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIO_Speaker_enable_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BTN_PWR_Pin */
	GPIO_InitStruct.Pin = BTN_POWER_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BTN_POWER_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : BTN_PAUSE_Pin BTN_GAME_Pin BTN_TIME_Pin */
	GPIO_InitStruct.Pin = BTN_PAUSE_Pin|BTN_GAME_Pin|BTN_TIME_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : PA4 PA5 PA6 */
	GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : PB12 */
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : PD8 PD1 PD4 */
	GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_1|GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/*Configure GPIO pins : BTN_A_Pin BTN_Left_Pin BTN_Down_Pin BTN_Right_Pin
	                         BTN_Up_Pin BTN_B_Pin */
	GPIO_InitStruct.Pin = BTN_A_Pin|BTN_Left_Pin|BTN_Down_Pin|BTN_Right_Pin
	                        |BTN_Up_Pin|BTN_B_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while(1) {
		// Blink display to indicate failure
		lcd_backlight_off();
		HAL_Delay(500);
		lcd_backlight_on();
		HAL_Delay(500);
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
