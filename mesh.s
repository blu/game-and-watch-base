// Utah VW Bug mesh, Copyright by Ivan Edward Sutherland et al
// https://commons.wikimedia.org/wiki/File:Utah_VW_Bug.stl

// Utah Teapot, Copyright by Martin Newell

	.section .sram_data.mesh_obj,"aw"
	.weak mesh_obj
	.type mesh_obj, %object
	.balign	16
mesh_obj:
#if mesh_alt == 5
	.incbin "mesh/british-museum-discobolus.bin"
#elif mesh_alt == 4
	.incbin "mesh/the-thinker-at-the-musee-lowpoly.bin"
#elif mesh_alt == 3
	.incbin "mesh/scan-the-world-michelangelo-s-david.bin"
#elif mesh_alt == 2
	.incbin "mesh/frog.bin"
#elif mesh_alt == 1
	.incbin "mesh/Utah_teapot.bin"
#else
	.incbin "mesh/Utah_VW_Bug.bin"
#endif
	// double the space for transformations
.equ	mesh_obj_size, . - mesh_obj - 2
	.space mesh_obj_size, 0x0
	.size mesh_obj, . - mesh_obj
