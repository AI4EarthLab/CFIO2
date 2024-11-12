cmake_minimum_required(VERSION 3.10)
project(MyProject LANGUAGES Fortran CXX)

# 启用 Fortran 支持
enable_language(Fortran)

# 设置 MPI 编译器
set(CMAKE_C_COMPILER mpicc)
set(CMAKE_CXX_COMPILER mpicxx)
set(CMAKE_Fortran_COMPILER mpif90)

# 添加编译选项 -O0 -g
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fopenmp -g")
set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -O3 -fopenmp -g")

# 指定包含目录
include_directories(include)

# 查找 src 和 examples 目录中的所有源文件
file(GLOB_RECURSE cpp_sources src/*.cpp)
file(GLOB_RECURSE f_sources fortran/*.F90)

# 从 Fortran 源文件创建一个库
add_library(fortran_interface ${f_sources} ${cpp_sources})

# 为每个示例源文件添加可执行文件
file(GLOB example_sources examples/*.cpp examples/*.F90)

foreach(example_source ${example_sources})
    get_filename_component(example_name ${example_source} NAME_WE)

    # 根据文件扩展名确定可执行文件的类型
    get_filename_component(extension ${example_source} EXT)
    
    add_executable(${example_name} ${example_source})

    # 将 Fortran 库链接到可执行文件
    target_link_libraries(${example_name} fortran_interface -lpnetcdf)

    # 设置链接器语言
    if(extension STREQUAL ".F90")
        set_target_properties(${example_name} PROPERTIES LINKER_LANGUAGE Fortran)
    elseif(extension STREQUAL ".cpp" OR extension STREQUAL ".c")
        set_target_properties(${example_name} PROPERTIES LINKER_LANGUAGE CXX)
    endif()
endforeach()