// Utah VW Bug mesh, Copyright by Ivan Edward Sutherland et al
// https://commons.wikimedia.org/wiki/File:Utah_VW_Bug.stl

	.section .data.mesh_obj
	.weak mesh_obj
	.type mesh_obj, %object
	.balign	32
	.space 30, 0x0
mesh_obj:
	.incbin "mesh/Utah_VW_Bug.bin"
	// double the space for transformations
.equ	mesh_obj_size, . - mesh_obj - 2
	.space mesh_obj_size, 0x0
	.size mesh_obj, . - mesh_obj
