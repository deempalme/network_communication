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
    pthread
  )

  target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

endif(NOT TARGET ${RamRodNetwork_LIBRARIES})
