cmake_minimum_required(VERSION 3.16)

# Custom partition table for OTA support
set(PARTITION_CSV_PATH ${CMAKE_CURRENT_SOURCE_DIR}/partitions.csv)

if(NOT DEFINED ENV{IDF_PATH})
    set(ENV{IDF_PATH} "C:/Users/user/esp/v5.5.1/esp-idf")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(aep_bill_code_x)

# Custom target: Generate SHA256 checksum AFTER binary is created
# This ensures the .bin file is fully written before hashing
add_custom_target(generate_sha256 ALL
    COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/tools/sec_calc_sha256.py ${CMAKE_BINARY_DIR}/aep_bill_code_x.bin
    COMMENT "Generating SHA256 checksum for OTA validation..."
    VERBATIM
)

# Make sure SHA256 generation happens AFTER the binary is created
add_dependencies(generate_sha256 gen_project_binary)
