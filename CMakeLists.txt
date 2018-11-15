cmake_minimum_required(VERSION 2.8.9)
project (gest)
set(SOURCE
    pulse.c
   )
include_directories(".")
add_executable(gest ${SOURCE})
TARGET_LINK_LIBRARIES(gest -lpigpio -lrt -lpthread)
