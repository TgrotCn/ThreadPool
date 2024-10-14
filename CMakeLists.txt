cmake_minimum_required(VERSION 3.0)


# 设置项目名称
project(ThreadPoolLib)

# 设置c__标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加目录变量
set(ROOT_DIR "${CMAKE_SOURCE_DIR}")
set(SRC_DIR "${ROOT_DIR}/src")
set(INCLUDE_DIR "${ROOT_DIR}/include")

# 获取指定目录下的所有源文件并添加到变量中
file(GLOB SRC_FILES "${SRC_DIR}/*.cpp")
# 创建共享库
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

# 设置输出库的名称和路径（可选）
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "threadpool")
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")