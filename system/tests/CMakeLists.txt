add_nodes(SRC_DIRS test_node TARGET_NAME test_nodes)
add_dependencies(node_system_test test_nodes)

install(FILES ${OUT_BINARY_DIR}/test_nodes.json
    DESTINATION bin/tests
)