cmake_minimum_required(VERSION 3.0)
project(BLIFMaker)

include_directories(${CMAKE_SOURCE_DIR}/include)

link_libraries(gvc)
link_libraries(cgraph)
link_libraries(cdt)



add_subdirectory(${CMAKE_SOURCE_DIR}/src)
