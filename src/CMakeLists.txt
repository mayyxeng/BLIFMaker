set(CMAKE_BUILD_TYPE Debug)


add_definitions(-DNO_VECTORIZATION)

#This is really bad now, I should change the library handling

set(SOURCES
    main.cpp
    Node.cpp
    #${opbitw}
   )
add_executable(BLIFMaker ${SOURCES})

# Find the libraries that correspond to the LLVM components
# that we wish to use
#llvm_map_components_to_libnames(llvm_libs support core irreader)
target_compile_options(BLIFMaker PUBLIC -O0 -std=c++11 -pedantic -Wall -fPIC)
# Link against LLVM libraries
target_link_libraries(BLIFMaker ${llvm_libs})

#get_target_property(BLIFMAKER_DIR BLIFMaker LOCATION)
add_custom_command(TARGET BLIFMaker
                   POST_BUILD
                 COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:BLIFMaker> "${CMAKE_CURRENT_BINARY_DIR}")
