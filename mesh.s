// Utah VW Bug mesh, Copyright by Ivan Edward Sutherland et al
// https://commons.wikimedia.org/wiki/File:Utah_VW_Bug.stl

// Utah Teapot, Copyright by Martin Newell

// Low Poly Frog, Copyright by Turtleman
// https://www.myminifactory.com/object/3d-print-low-poly-frog-50813

// Low Poly Howling Wolf, Copyright by Brett Poultney
// https://www.printables.com/model/386392-low-poly-howling-wolf-20-decorationno-supports

// Infinity Cube, Copyright by Robert Mosby
// https://www.thingiverse.com/thing:3496723

	.section .sram_data.mesh_obj_0,"aw"
	.weak mesh_obj_0
	.type mesh_obj_0, %object
	.balign	16
mesh_obj_0:
	.incbin "mesh/Utah_VW_Bug.bin"
.equ	mesh_obj_0_size, . - mesh_obj_0 - 2
	.size mesh_obj_0, . - mesh_obj_0

	.section .sram_data.mesh_obj_1,"aw"
	.weak mesh_obj_1
	.type mesh_obj_1, %object
	.balign	16
mesh_obj_1:
	.incbin "mesh/Utah_teapot.bin"
.equ	mesh_obj_1_size, . - mesh_obj_1 - 2
	.size mesh_obj_1, . - mesh_obj_1

	.section .sram_data.mesh_obj_2,"aw"
	.weak mesh_obj_2
	.type mesh_obj_2, %object
	.balign	16
mesh_obj_2:
	.incbin "mesh/frog.bin"
.equ	mesh_obj_2_size, . - mesh_obj_2 - 2
	.size mesh_obj_2, . - mesh_obj_2

	.section .sram_data.mesh_obj_3,"aw"
	.weak mesh_obj_3
	.type mesh_obj_3, %object
	.balign	16
mesh_obj_3:
	.incbin "mesh/wolf.bin"
.equ	mesh_obj_3_size, . - mesh_obj_3 - 2
	.size mesh_obj_3, . - mesh_obj_3

	.section .sram_data.mesh_obj_4,"aw"
	.weak mesh_obj_4
	.type mesh_obj_4, %object
	.balign	16
mesh_obj_4:
	.incbin "mesh/david.bin"
.equ	mesh_obj_4_size, . - mesh_obj_4 - 2
	.size mesh_obj_4, . - mesh_obj_4

	.section .sram_data.mesh_obj_5,"aw"
	.weak mesh_obj_5
	.type mesh_obj_5, %object
	.balign	16
mesh_obj_5:
	.incbin "mesh/discobolus.bin"
.equ	mesh_obj_5_size, . - mesh_obj_5 - 2
	.size mesh_obj_5, . - mesh_obj_5

	.section .sram_data.mesh_obj_6,"aw"
	.weak mesh_obj_6
	.type mesh_obj_6, %object
	.balign	16
mesh_obj_6:
	.incbin "mesh/infinity_cube.bin"
.equ	mesh_obj_6_size, . - mesh_obj_6 - 2
	.size mesh_obj_6, . - mesh_obj_6

	.section .bss.mesh_scr
	.weak mesh_scr
	.type mesh_scr, %object
.equ	mesh_size_max, mesh_obj_0_size

.if	mesh_size_max < mesh_obj_1_size
.equ	mesh_size_max, mesh_obj_1_size
.endif
.if	mesh_size_max < mesh_obj_2_size
.equ	mesh_size_max, mesh_obj_2_size
.endif
.if	mesh_size_max < mesh_obj_3_size
.equ	mesh_size_max, mesh_obj_3_size
.endif
.if	mesh_size_max < mesh_obj_4_size
.equ	mesh_size_max, mesh_obj_4_size
.endif
.if	mesh_size_max < mesh_obj_5_size
.equ	mesh_size_max, mesh_obj_5_size
.endif
.if	mesh_size_max < mesh_obj_6_size
.equ	mesh_size_max, mesh_obj_6_size
.endif
	.balign	16
mesh_scr:
	.space mesh_size_max, 0x0
	.size mesh_scr, . - mesh_scr
