// Fonts from the VGA-ROMs repository
// https://github.com/spacerace/romfont

	.section .data.fnt_wang_8x8
	.weak fnt_wang_8x8
	.type fnt_wang_8x8, %object
	.balign	8
fnt_wang_8x8:
	.incbin "font/wang_3050_BIOS_ROM__8x8.bin"
	.size fnt_wang_8x8, . - fnt_wang_8x8

	.section .data.fnt_wang_8x16
	.weak fnt_wang_8x16
	.type fnt_wang_8x16, %object
	.balign	8
fnt_wang_8x16:
	.incbin "font/wang_3050_BIOS_ROM__8x16.bin"
	.size fnt_wang_8x16, . - fnt_wang_8x16
