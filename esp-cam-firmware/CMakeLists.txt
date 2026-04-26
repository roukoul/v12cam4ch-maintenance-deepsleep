cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{IDF_PATH})
    set(ENV{IDF_PATH} "C:/Users/user/esp/v5.5.1/esp-idf")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-cam-firmware)
