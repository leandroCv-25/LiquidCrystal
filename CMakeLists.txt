include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(srcs "src/liquid_crystal.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "driver")