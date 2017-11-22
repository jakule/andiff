include_directories(${CMAKE_SOURCE_DIR})
file(GLOB_RECURSE INTERNAL_INCLUDES "${CMAKE_SOURCE_DIR}/*.hpp")

add_executable(${PATCH_EXE_NAME} anpatch.cpp)
target_link_libraries(${PATCH_EXE_NAME}
    ${CMAKE_THREAD_LIBS_INIT}
    ${BZIP2_LIBRARIES})

include_directories(${LIBDIVSUFSORT_INCLUDE_DIR})
add_executable(${DIFF_EXE_NAME} andiff.cpp ${INTERNAL_INCLUDES})
target_link_libraries(${DIFF_EXE_NAME} ${CMAKE_THREAD_LIBS_INIT}
    ${BZIP2_LIBRARIES}
    ${LIBDIVSUFSORT_LIBRARY}
    ${LIBDIVSUFSORT64_LIBRARY}
    ${OpenMP_CXX_FLAGS})

install(TARGETS ${DIFF_EXE_NAME} ${PATCH_EXE_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)