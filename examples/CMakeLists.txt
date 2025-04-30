cmake_minimum_required(VERSION 3.10)
project(JFetchExamples)

find_package(CURL REQUIRED)

file(GLOB EXAMPLE_SOURCES "*.cpp")

foreach(EXAMPLE ${EXAMPLE_SOURCES})
  get_filename_component(EXEC_NAME ${EXAMPLE} NAME_WE)
  add_executable(${EXEC_NAME} ${EXAMPLE})
  target_link_libraries(${EXEC_NAME} PRIVATE CURL::libcurl)
endforeach()
