UCG_ADD_APP(
    SRC run_server.cpp
    LIBS web_server nodes_system httplib::httplib
)

# 设置路径
set(WEB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web")
set(WEB_DEST_DIR "${OUT_BINARY_DIR}/web/dist")
set(WEB_DIST_DIR "${WEB_SOURCE_DIR}/dist")

# 查找 npm
find_program(NPM_EXECUTABLE npm)
if(NPM_EXECUTABLE)
    # 设置跟踪文件
    set(NPM_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/npm.stamp")
    set(BUILD_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/build.stamp")

    # npm install 依赖文件（package.json）
    set(INSTALL_DEP_FILES 
        "${WEB_SOURCE_DIR}/package.json"
    )

    # npm run build 依赖文件（src文件夹、index.html、vite.config.js）
    # 注意：排除dist目录，避免循环依赖
    file(GLOB_RECURSE SRC_FILES "${WEB_SOURCE_DIR}/src/*")
    set(BUILD_DEP_FILES 
        ${SRC_FILES}
        "${WEB_SOURCE_DIR}/index.html"
        "${WEB_SOURCE_DIR}/vite.config.js"
    )
    
    # 添加调试信息
    # message(STATUS "BUILD_DEP_FILES: ${BUILD_DEP_FILES}")
    # message(STATUS "NPM_STAMP_FILE: ${NPM_STAMP_FILE}")
    # message(STATUS "BUILD_STAMP_FILE: ${BUILD_STAMP_FILE}")

    # npm install 自定义命令
    add_custom_command(
        OUTPUT ${NPM_STAMP_FILE}
        # 先创建标记文件，再执行npm install，否则无法创建标记文件
        COMMAND ${CMAKE_COMMAND} -E touch ${NPM_STAMP_FILE}
        COMMAND ${NPM_EXECUTABLE} install
        WORKING_DIRECTORY ${WEB_SOURCE_DIR}
        DEPENDS ${INSTALL_DEP_FILES}
        COMMENT "Installing npm dependencies"
    )

    # npm run build 自定义命令
    add_custom_command(
        OUTPUT ${BUILD_STAMP_FILE}
        # 先创建标记文件，再执行npm install，否则无法创建标记文件
        COMMAND ${CMAKE_COMMAND} -E touch ${BUILD_STAMP_FILE}
        COMMAND ${NPM_EXECUTABLE} run build
        WORKING_DIRECTORY ${WEB_SOURCE_DIR}
        DEPENDS ${BUILD_DEP_FILES} ${NPM_STAMP_FILE}
        COMMENT "Building frontend"
    )

    # 前端构建目标
    add_custom_target(build_frontend DEPENDS ${BUILD_STAMP_FILE})
    add_dependencies(run_server.cpp build_frontend)

    # 复制构建结果
    add_custom_command(
        TARGET run_server.cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
                "${WEB_DIST_DIR}" "${WEB_DEST_DIR}"
        COMMENT "Copying web assets to ${WEB_DEST_DIR}"
        VERBATIM
    )

    message(STATUS "NPM found: ${NPM_EXECUTABLE}")
else()
    # 无 npm 时的回退方案
    add_custom_command(
        TARGET run_server.cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
                "${WEB_SOURCE_DIR}" "${WEB_DEST_DIR}"
        COMMENT "Copying raw web files (npm not available)"
        VERBATIM
    )
    message(WARNING "npm not found! Using raw web files. For production build, run 'npm install && npm run build' in ${WEB_SOURCE_DIR}")
endif()