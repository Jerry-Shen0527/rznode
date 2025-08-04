
USTC_CG_ADD_LIB(
	nodes_ui_imgui SHARED
	PUBLIC_LIBS GUI nodes_core RHI blueprints
	PRIVATE_LIBS glfw spdlog::spdlog stb_image nodes_system test_nodes
)
