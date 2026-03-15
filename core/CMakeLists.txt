RUZINO_ADD_LIB(
    nodes_core
    SHARED
    INC_DIR ${PROJECT_SOURCE_DIR}/include/api ${PROJECT_SOURCE_DIR}/include/Utils
    PUBLIC_LIBS EnTT::EnTT spdlog::spdlog spdlog::spdlog_header_only
    PYTHON_WRAP_DIR python
)

# Install nodes headers
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/nodes
    DESTINATION include/nodes
    FILES_MATCHING PATTERN "*.hpp"
)
