// Utah VW Bug mesh, Copyright by Ivan Edward Sutherland et al
// https://commons.wikimedia.org/wiki/File:Utah_VW_Bug.stl

// Utah Teapot, Copyright by Martin Newell

	.section .data.mesh_obj
	.weak mesh_obj
	.type mesh_obj, %object
	.balign	16
mesh_obj:
#if mesh_alt == 1
	.incbin "mesh/Utah_teapot.bin"
#else
	.incbin "mesh/Utah_VW_Bug.bin"
#endif
	// double the space for transformations
.equ	mesh_obj_size, . - mesh_obj - 2
	.space mesh_obj_size, 0x0
	.size mesh_obj, . - mesh_obj
