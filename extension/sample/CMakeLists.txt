project(wxex-sample)

file(GLOB SRCS "*.cpp")

if(WIN32)
  set(SRCS ${SRCS} app.rc)
endif(WIN32)
  
include(../../cmake/common.cmake)

add_executable(
  ${PROJECT_NAME}
  WIN32 
  ${SRCS})
  
target_link_all()

add_subdirectory(report)
