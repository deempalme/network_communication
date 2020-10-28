#cmake version
cmake_minimum_required(VERSION 3.12...3.15)

if(NOT TARGET ${RamRodNetwork_LIBRARIES})
  project(RamRodNetwork
    VERSION 0.8.0
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

  #add every file to the executable
  add_library(${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD          17
    CXX_STANDARD_REQUIRED TRUE
    CXX_EXTENSIONS        FALSE
  )

  target_sources(${PROJECT_NAME}
    PRIVATE
    #src/ramrod/network_communication/client.cpp
    src/ramrod/network_communication/server.cpp
    )

  target_include_directories(${PROJECT_NAME} BEFORE PRIVATE include )

  target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

  target_compile_options(${PROJECT_NAME}
    PRIVATE
      -Wall
      -Wextra
      -Wundef
      -pedantic
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
