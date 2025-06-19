#cmake version
cmake_minimum_required(VERSION 3.15)

if(NOT TARGET ${RamRodNetwork_LIBRARIES})
    project(RamRodNetwork
        VERSION 1.0.0
        DESCRIPTION "Ram-Rod's Network communication library"
        LANGUAGES CXX C
    )

    set(${PROJECT_NAME}_INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/include
        CACHE INTERNAL ""
    )

    set(${PROJECT_NAME}_LIBRARIES
        ${PROJECT_NAME}
        CACHE INTERNAL ""
    )

    set(INC_DIR include/ramrod/socket)
    set(SRC_DIR src/ramrod/socket)

    #header files
    set(UNIT_HPP_FILES
        ${INC_DIR}/BasicSocket.hpp
        ${INC_DIR}/Client.hpp
        ${INC_DIR}/Conversor.hpp
        ${INC_DIR}/Enumerators.hpp
        ${INC_DIR}/ErrorHandler.hpp
        ${INC_DIR}/Server.hpp
    )

    #source files
    set(UNIT_CPP_FILES
        ${SRC_DIR}/BasicSocket.cpp
        ${SRC_DIR}/Client.cpp
        ${SRC_DIR}/Conversor.cpp
        ${SRC_DIR}/ErrorHandler.cpp
        ${SRC_DIR}/Server.cpp
    )

    #add every file to the executable
    add_library(${PROJECT_NAME})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED TRUE
        CXX_EXTENSIONS FALSE
    )

    target_sources(${PROJECT_NAME}
        PRIVATE
        ${UNIT_CPP_FILES}
        ${UNIT_HPP_FILES}
    )

    target_include_directories(${PROJECT_NAME} BEFORE
        PRIVATE
        include
    )

    target_link_libraries(${PROJECT_NAME}
        pthread
    )

    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

    target_compile_options(${PROJECT_NAME}
        PRIVATE
        -Wall
        -Wextra
        -Wundef
        -Wpedantic
        -Wconversion
        -Wcast-align
        -Wfloat-equal
        -Wpointer-arith
        -Wsign-conversion
        -Wunreachable-code
        -Wunused-function
        -Wignored-qualifiers
    )
endif(NOT TARGET ${RamRodNetwork_LIBRARIES})
