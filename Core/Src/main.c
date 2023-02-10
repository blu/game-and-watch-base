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
#include "main.h"

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

int32_t mul_sin(int16_t multiplicand, uint16_t ticks) __attribute__ ((naked));

int32_t mul_sin(int16_t multiplicand, uint16_t ticks)
{
	__asm__ (
	/*
	  multiply by sine
	  r0.w: multiplicand
	  r1: angle ticks -- [0, 2pi) -> [0, 256)
	  returns: r0: sine product as fx16.15 (r0[31] replicates sign)
	  clobbers: r12
	*/
		"ands	r1,r1,#0xff\n\t"
		"cmp	r1,#0x80\n\t"
		"bcc	sign_done\n\t"
		"negs	r0,r0\n\t"
		"subs	r1,r1,#0x80\n\t"
	"sign_done:\n\t"
		"cmp	r1,#0x40\n\t"
		"bcc	fetch\n\t"
		"bne	not_maximum\n\t"
		"sxth	r0,r0\n\t"
		"lsls	r0,r0,#15\n\t"
		"bx	lr\n\t"
	"not_maximum:\n\t"
		"subs	r1,r1,#0x80\n\t"
		"negs	r1,r1\n\t"
	"fetch:\n\t"
		"adr	r12,sinLUT15_64\n\t"
		"ldrh	r1,[r12,r1,lsl #1]\n\t"
		"smulbb	r0,r0,r1\n\t"
		"bx	lr\n\t"
		".balign 32\n\t"
	"sinLUT15_64:\n\t"
		".short 0x0000, 0x0324, 0x0648, 0x096B, 0x0C8C, 0x0FAB, 0x12C8, 0x15E2\n\t"
		".short 0x18F9, 0x1C0C, 0x1F1A, 0x2224, 0x2528, 0x2827, 0x2B1F, 0x2E11\n\t"
		".short 0x30FC, 0x33DF, 0x36BA, 0x398D, 0x3C57, 0x3F17, 0x41CE, 0x447B\n\t"
		".short 0x471D, 0x49B4, 0x4C40, 0x4EC0, 0x5134, 0x539B, 0x55F6, 0x5843\n\t"
		".short 0x5A82, 0x5CB4, 0x5ED7, 0x60EC, 0x62F2, 0x64E9, 0x66D0, 0x68A7\n\t"
		".short 0x6A6E, 0x6C24, 0x6DCA, 0x6F5F, 0x70E3, 0x7255, 0x73B6, 0x7505\n\t"
		".short 0x7642, 0x776C, 0x7885, 0x798A, 0x7A7D, 0x7B5D, 0x7C2A, 0x7CE4\n\t"
		".short 0x7D8A, 0x7E1E, 0x7E9D, 0x7F0A, 0x7F62, 0x7FA7, 0x7FD9, 0x7FF6"
	);
}

int32_t mul_sin2(int16_t multiplicand, uint16_t ticks) __attribute__ ((naked));

int32_t mul_sin2(int16_t multiplicand, uint16_t ticks)
{
	__asm__ (
	/*
	  multiply by sine
	  r0.w: multiplicand
	  r1: angle ticks -- [0, 2pi) -> [0, 256)
	  returns: r0: sine product as fx16.15 (r0[31] replicates sign)
	  clobbers: r12
	*/
		"ands	r1,r1,#0xff\n\t"
		"cmp	r1,#0x80\n\t"
		"bcc	sign_done2\n\t"
		"negs	r0,r0\n\t"
		"subs	r1,r1,#0x80\n\t"
	"sign_done2:\n\t"
		"cmp	r1,#0x40\n\t"
		"bne	fetch2\n\t"
		"sxth	r0,r0\n\t"
		"lsls	r0,r0,#15\n\t"
		"bx	lr\n\t"
	"fetch2:\n\t"
		"adr	r12,sinLUT15_64_2\n\t"
		"ldrh	r1,[r12,r1,lsl #1]\n\t"
		"smulbb	r0,r0,r1\n\t"
		"bx	lr\n\t"
		".balign 32\n\t"
	"sinLUT15_64_2:\n\t"
		".short 0x0000, 0x0324, 0x0648, 0x096B, 0x0C8C, 0x0FAB, 0x12C8, 0x15E2\n\t"
		".short 0x18F9, 0x1C0C, 0x1F1A, 0x2224, 0x2528, 0x2827, 0x2B1F, 0x2E11\n\t"
		".short 0x30FC, 0x33DF, 0x36BA, 0x398D, 0x3C57, 0x3F17, 0x41CE, 0x447B\n\t"
		".short 0x471D, 0x49B4, 0x4C40, 0x4EC0, 0x5134, 0x539B, 0x55F6, 0x5843\n\t"
		".short 0x5A82, 0x5CB4, 0x5ED7, 0x60EC, 0x62F2, 0x64E9, 0x66D0, 0x68A7\n\t"
		".short 0x6A6E, 0x6C24, 0x6DCA, 0x6F5F, 0x70E3, 0x7255, 0x73B6, 0x7505\n\t"
		".short 0x7642, 0x776C, 0x7885, 0x798A, 0x7A7D, 0x7B5D, 0x7C2A, 0x7CE4\n\t"
		".short 0x7D8A, 0x7E1E, 0x7E9D, 0x7F0A, 0x7F62, 0x7FA7, 0x7FD9, 0x7FF6\n\t"
		".short 0x0000, 0x7FF6, 0x7FD9, 0x7FA7, 0x7F62, 0x7F0A, 0x7E9D, 0x7E1E\n\t"
		".short 0x7D8A, 0x7CE4, 0x7C2A, 0x7B5D, 0x7A7D, 0x798A, 0x7885, 0x776C\n\t"
		".short 0x7642, 0x7505, 0x73B6, 0x7255, 0x70E3, 0x6F5F, 0x6DCA, 0x6C24\n\t"
		".short 0x6A6E, 0x68A7, 0x66D0, 0x64E9, 0x62F2, 0x60EC, 0x5ED7, 0x5CB4\n\t"
		".short 0x5A82, 0x5843, 0x55F6, 0x539B, 0x5134, 0x4EC0, 0x4C40, 0x49B4\n\t"
		".short 0x471D, 0x447B, 0x41CE, 0x3F17, 0x3C57, 0x398D, 0x36BA, 0x33DF\n\t"
		".short 0x30FC, 0x2E11, 0x2B1F, 0x2827, 0x2528, 0x2224, 0x1F1A, 0x1C0C\n\t"
		".short 0x18F9, 0x15E2, 0x12C8, 0x0FAB, 0x0C8C, 0x096B, 0x0648, 0x0324"
	);
}

int32_t mul_cos(int16_t multiplicand, uint16_t ticks) __attribute__ ((naked));

int32_t mul_cos(int16_t multiplicand, uint16_t ticks)
{
	__asm__ (
	/*
	  multiply by cosine
	  r0.w: multiplicand
	  r1: angle ticks -- [0, 2pi) -> [0, 256)
	  returns; r0: cosine product as fx16.15 (r0[31] replicates sign)
	  clobbers: r12
	*/
		"adds	r1,r1,#0x40\n\t"
		"b	mul_sin\n\t"
	);
}

int32_t mul_cos2(int16_t multiplicand, uint16_t ticks) __attribute__ ((naked));

int32_t mul_cos2(int16_t multiplicand, uint16_t ticks)
{
	__asm__ (
	/*
	  multiply by cosine
	  r0.w: multiplicand
	  r1: angle ticks -- [0, 2pi) -> [0, 256)
	  returns; r0: cosine product as fx16.15 (r0[31] replicates sign)
	  clobbers: r12
	*/
		"adds	r1,r1,#0x40\n\t"
		"b	mul_sin2\n\t"
	);
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

	lcd_init(&hspi2);
	memset(framebuffer, 0, sizeof(framebuffer));

	HAL_LTDC_SetAddress(&hltdc, (uint32_t) &framebuffer[1], LTDC_LAYER_1);

	/* USER CODE END 2 */

	flash_memory_map(&hospi1);

	// Sanity check, sometimes this is triggered
	uint32_t add = 0x90000000;
	uint32_t* ptr = (uint32_t*)add;
	if(*ptr == 0x88888888) {
		Error_Handler();
	}

	uint8_t alt = 0;
	uint16_t color = 0;
	uint32_t i = 0, last_press = 0;

	while (1) {
		uint32_t buttons = buttons_get();
		if(buttons & B_Left) {
			color = 0x1f << (5 + 6 + 0);
		}
		if(buttons & B_Right) {
			color = 0x3f << (5 + 0 + 0);
		}
		if(buttons & B_Up) {
			color = 0x1f << (0 + 0 + 0);
		}
		if(buttons & B_Down) {
			color = 0;
		}
		if (buttons & B_A && i - last_press > 7) {
			last_press = i;
			alt += alt < 4 ? 1 : 0;
		}
		if (buttons & B_B && i - last_press > 7) {
			last_press = i;
			alt -= alt > 0 ? 1 : 0;
		}

		if (alt == 0) {
			/* Checkers of the color */
			int32_t off = mul_sin(64, i * 4);
			__asm__ __volatile (
				"asrs	%[arg],%[arg],#15\n\t"
				"adcs	%[arg],%[arg],#0\n\t"
			: [arg] "=r" (off) : "0" (off));

			for(int y=0, row=0; y < 240; y++, row+=320) {
				for(int x=0; x < 320; x++) {
					if(((x + off) & 32) ^ (((y + off) & 32))) {
						framebuffer[i & 1][row+x] = color;
					} else {
						framebuffer[i & 1][row+x] = 0xffff;
					}
				}
			}
		}
		else if (alt == 1) {
			/* Uniform squares of the color */
			int32_t off_x = mul_sin(64, i * 4);
			int32_t off_y = mul_cos(128, i * 2);
			__asm__ __volatile (
				"asrs	%[arg_x],%[arg_x],#15\n\t"
				"adcs	%[arg_x],%[arg_x],#0\n\t"
				"asrs	%[arg_y],%[arg_y],#15\n\t"
				"adcs	%[arg_y],%[arg_y],#0\n\t"
			: [arg_x] "=r" (off_x),
			  [arg_y] "=r" (off_y)
			: "0" (off_x),
			  "1" (off_y));

			for(int y=0, row=0; y < 240; y++, row+=320) {
				for(int x=0; x < 320; x++) {
					if(((x + off_x) & 32) & (((y + off_y) & 32))) {
						framebuffer[i & 1][row+x] = color;
					} else {
						framebuffer[i & 1][row+x] = 0xffff;
					}
				}
			}
		}
		else if (alt == 2) {
			for(int y=0, row=0; y < 240; y++, row+=320) {
				for(int x=0; x < 320; x++) {
					int32_t off_x = mul_sin(0xf, (x + i * 4) * 4);
					int32_t off_y = mul_cos(0xf, (y + i * 4) * 4);
					__asm__ __volatile (
						"asrs	%[arg_x],%[arg_x],#15\n\t"
						"adcs	%[arg_x],%[arg_x],#16\n\t"
						"asrs	%[arg_y],%[arg_y],#15\n\t"
						"adcs	%[arg_y],%[arg_y],#16\n\t"
					: [arg_x] "=r" (off_x),
					  [arg_y] "=r" (off_y)
					: "0" (off_x),
					  "1" (off_y));

					framebuffer[i & 1][row+x] = off_x + off_y;
				}
			}
		}
		else if (alt == 3) {
			/* Inverse dot circling CW on color bg */
			register uint16_t val_color asm ("r0") = color;
			register uint32_t val_i asm ("r11") = i;
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
			/* plot CW-rotating dots */
				"movs	r3,#0xff\n\t"
			"2:\n\t"
				"movs	r0,#112\n\t"
				"adds	r1,r3,%[idx]\n\t"
				"bl	mul_sin\n\t"
				"asrs	r0,r0,#15\n\t"
				"adcs	r0,r0,#120\n\t"
				"movs	r1,#640\n\t"
				"smulbb	r2,r1,r0\n\t"

				"movs	r0,#112\n\t"
				"adds	r1,r3,%[idx]\n\t"
				"bl	mul_cos\n\t"
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
			: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "cc");
		}
		else {
			/* Inverse line circling CW on color bg */
			register uint16_t val_color asm ("r0") = color;
			register uint32_t val_i asm ("r11") = i;
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
			/* plot lines */
				"ldr	r8,[sp]\n\t"
				"mvns	r8,r8\n\t"
				"movs	r10,#0xff\n\t"
			"2:\n\t"
				"movs	r0,#112\n\t"
				"rsbs	r1,r10,#0xff\n\t"
				"adds	r1,r1,%[idx]\n\t"
				"bl	mul_sin\n\t"
				"asrs	r0,r0,#15\n\t"
				"adcs	r2,r0,#120\n\t"
				"rsbs	r3,r0,#120\n\t"

				"movs	r0,#112\n\t"
				"rsbs	r1,r10,#0xff\n\t"
				"adds	r1,r1,%[idx]\n\t"
				"bl	mul_cos\n\t"
				"asrs	r0,r0,#15\n\t"
				"adcs	r4,r0,#160\n\t"

				"movs	r1,r2\n\t"
				"rsbs	r2,r0,#160\n\t"
				"movs	r0,r4\n\t"
				"ldr	%[fb],[sp,#4]\n\t"
				"bl	line\n\t"

				"subs	r10,r10,#8\n\t"
				"bcs	2b\n\t"

				"ldmia	sp!,{%[color],%[fb]}\n\t"
				"b	done\n\t"
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
			"line:\n\t"
			/* compute x0,y0 addr in fb */
				"movs	r4,#640\n\t"
				"smulbb	r4,r4,r1\n\t"
				"adds	r12,r12,r4\n\t"
				"adds	r12,r12,r0,lsl #1\n\t"

				"movs	r6,#1\n\t"
				"subs	r4,r2,r0\n\t" /* dx */
				"bge	dx_done\n\t"
				"negs	r4,r4\n\t"
				"negs	r6,r6\n\t"
			"dx_done:\n\t"
				"movs	r7,#1\n\t"
				"movs	r9,#640\n\t"
				"subs	r5,r3,r1\n\t" /* dy */
				"bge	dy_done\n\t"
				"negs	r5,r5\n\t"
				"negs	r7,r7\n\t"
				"negs	r9,r9\n\t"
			"dy_done:\n\t"
				"cmp	r5,r4\n\t"
				"bge	high_slope\n\t"
			/* low slope: iterate along x */
				"lsls	r5,r5,#1\n\t" /* 2 dy */
				"movs	r3,r5\n\t"
				"subs	r3,r3,r4\n\t" /* 2 dy - dx */
				"lsls	r4,r4,#1\n\t" /* 2 dx */
			"loop_x:\n\t"
				"strh	r8,[r12]\n\t"
			"advance_x:\n\t"
				"adds	r12,r12,r6,lsl #1\n\t"
				"adds	r0,r0,r6\n\t"
				"tst	r3,r3\n\t"
				"ble	x_done\n\t"
				"adds	r12,r12,r9\n\t"
				"subs	r3,r3,r4\n\t"
			"x_done:\n\t"
				"adds	r3,r3,r5\n\t"
				"cmp	r0,r2\n\t"
				"bne	loop_x\n\t"
				"bx	lr\n\t"
			"high_slope:\n\t" /* iterate along y */
				"lsls	r4,r4,#1\n\t" /* 2 dx */
				"movs	r2,r4\n\t"
				"subs	r2,r2,r5\n\t" /* 2 dx - dy */
				"lsls	r5,r5,#1\n\t" /* 2 dy */
				"bne	loop_y\n\t"
				"bx	lr\n\t"
			"loop_y:\n\t"
				"strh	r8,[r12]\n\t"
			"advance_y:\n\t"
				"adds	r12,r12,r9\n\t"
				"adds	r1,r1,r7\n\t"
				"tst	r2,r2\n\t"
				"ble	y_done\n\t"
				"adds	r12,r12,r6,lsl #1\n\t"
				"subs	r2,r2,r5\n\t"
			"y_done:\n\t"
				"adds	r2,r2,r4\n\t"
				"cmp	r1,r3\n\t"
				"bne	loop_y\n\t"
				"bx	lr\n\t"
			"done:"
			: /* none */
			: [color] "r" (val_color),
			  [idx] "r" (val_i),
			  [fb] "r" (ptr_fb)
			: "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			  "r9", "r10", "cc");
		}

		HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t) &framebuffer[i & 1], LTDC_LAYER_1);
		HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);

		while ((hltdc.Instance->SRCR & LTDC_RELOAD_VERTICAL_BLANKING) != 0) {}

		i++;
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
