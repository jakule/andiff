include_directories(${CMAKE_SOURCE_DIR})
file(GLOB_RECURSE INTERNAL_INCLUDES "${CMAKE_SOURCE_DIR}/*.hpp")

add_executable(${PATCH_EXE_NAME} anpatch.cpp)
target_link_libraries(${PATCH_EXE_NAME}
    ${CMAKE_THREAD_LIBS_INIT}
    ${BZIP2_LIBRARIES})

include_directories(${BZIP2_INCLUDE_DIR})
include_directories(${LIBDIVSUFSORT_INCLUDE_DIR})
add_executable(${DIFF_EXE_NAME} andiff.cpp ${INTERNAL_INCLUDES})
target_link_libraries(${DIFF_EXE_NAME} ${CMAKE_THREAD_LIBS_INIT}
    ${BZIP2_LIBRARIES}
    ${LIBDIVSUFSORT_LIBRARY}
    ${LIBDIVSUFSORT64_LIBRARY})

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(${DIFF_EXE_NAME} ${OpenMP_CXX_FLAGS})
elseif(MSVC)
    add_compile_options(${OpenMP_CXX_FLAGS})
endif()
