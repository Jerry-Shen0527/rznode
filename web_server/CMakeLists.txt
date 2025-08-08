USTC_CG_ADD_LIB(
    web_server
    SHARED
    PUBLIC_LIBS nodes_core nodes_system
    PRIVATE_LIBS httplib::httplib
)

# 设置路径
set(WEB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web")
set(WEB_DEST_DIR "${OUT_BINARY_DIR}/web")
set(WEB_DIST_DIR "${WEB_SOURCE_DIR}/dist")

# 查找 npm
find_program(NPM_EXECUTABLE npm)
if(NPM_EXECUTABLE)
    # 设置跟踪文件
    set(NPM_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/npm.stamp")
    set(BUILD_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/build.stamp")

    # 定义依赖文件（只依赖package.json，避免package-lock.json的频繁变化）
    set(DEP_FILES 
        "${WEB_SOURCE_DIR}/package.json"
    )

    # npm install 自定义命令
    add_custom_command(
        OUTPUT ${NPM_STAMP_FILE}
        # 先创建标记文件，再执行npm install，否则无法创建标记文件
        COMMAND ${CMAKE_COMMAND} -E touch ${NPM_STAMP_FILE}
        COMMAND ${NPM_EXECUTABLE} install
        WORKING_DIRECTORY ${WEB_SOURCE_DIR}
        DEPENDS ${DEP_FILES}
        COMMENT "Installing npm dependencies"
    )

    # npm run build 自定义命令
    add_custom_command(
        OUTPUT ${BUILD_STAMP_FILE}
        # 先创建标记文件，再执行npm install，否则无法创建标记文件
        COMMAND ${CMAKE_COMMAND} -E touch ${BUILD_STAMP_FILE}
        COMMAND ${NPM_EXECUTABLE} run build
        WORKING_DIRECTORY ${WEB_SOURCE_DIR}
        DEPENDS ${NPM_STAMP_FILE}
        COMMENT "Building frontend"
    )

    # 前端构建目标
    add_custom_target(build_frontend DEPENDS ${BUILD_STAMP_FILE})
    add_dependencies(web_server build_frontend)

    # 复制构建结果
    add_custom_command(
        TARGET web_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
                "${WEB_DIST_DIR}" "${WEB_DEST_DIR}"
        COMMENT "Copying web assets to ${WEB_DEST_DIR}"
        VERBATIM
    )

    message(STATUS "NPM found: ${NPM_EXECUTABLE}")
else()
    # 无 npm 时的回退方案
    add_custom_command(
        TARGET web_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
                "${WEB_SOURCE_DIR}" "${WEB_DEST_DIR}"
        COMMENT "Copying raw web files (npm not available)"
        VERBATIM
    )
    message(WARNING "npm not found! Using raw web files. For production build, run 'npm install && npm run build' in ${WEB_SOURCE_DIR}")
endif()