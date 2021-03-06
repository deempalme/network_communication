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

  # +++++++++++++++++++++++++++++++++++++ Console printer ++++++++++++++++++++++++++++++++++++
  # adding the root directory of torero source tree to your project
  add_subdirectory(lib/ramrod/console_printer)

  #add every file to the executable
  add_library(${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD          17
    CXX_STANDARD_REQUIRED TRUE
    CXX_EXTENSIONS        FALSE
  )

  target_sources(${PROJECT_NAME}
    PRIVATE
      src/ramrod/network_communication/conversor.cpp
      src/ramrod/network_communication/client.cpp
      src/ramrod/network_communication/server.cpp
  )

  target_include_directories(${PROJECT_NAME} BEFORE
    PRIVATE
      include
      ${RamRodConsole_INCLUDE_DIRS}
  )

  target_link_libraries(${PROJECT_NAME}
    ${RamRodConsole_LIBRARIES}
  )

  target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

  # TODO: delete this after finishing code
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(WARNING "Compiling warnings in debug mode: ${CMAKE_BUILD_TYPE}")

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

    ##########################
    ## include-what-you-use ##
    ##########################

    find_program(iwyu_path NAMES include-what-you-use iwyu)
    if(NOT iwyu_path)
      message(FATAL_ERROR "Could not find the program include-what-you-use")
    else()
      message(STATUS "include-what-you-use found: ${iwyu_path}")
      get_filename_component(PARENT_DIR ${iwyu_path} DIRECTORY)
      get_filename_component(G_PARENT_DIR ${PARENT_DIR} DIRECTORY)
      set(iwyu_share_path ${G_PARENT_DIR}/share/include-what-you-use)
      message("iwyu share path: ${iwyu_share_path}")
      set(iwyu_path_and_options
        ${iwyu_path}
        -Xiwyu
        --mapping_file=${iwyu_share_path}/boost-all.imp
        -Xiwyu
        --mapping_file=${iwyu_share_path}/boost-all-private.imp
      )
      set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        CXX_INCLUDE_WHAT_YOU_USE
        "${iwyu_path_and_options}"
      )
    endif()
  endif(CMAKE_BUILD_TYPE MATCHES Debug)

endif(NOT TARGET ${RamRodNetwork_LIBRARIES})
