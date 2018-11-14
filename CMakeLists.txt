cmake_minimum_required(VERSION 2.8.9)
project (hello)
set(SOURCE
    pulse.c
   )
include_directories(".")
add_executable(hello ${SOURCE})
TARGET_LINK_LIBRARIES(hello -lpigpio -lrt -lpthread)
