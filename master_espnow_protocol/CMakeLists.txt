# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
<<<<<<< HEAD
project(slave_espnow_protocol)

list(APPEND EXTRA_COMPONENT_DIRS "components")
=======
project(master_espnow_protocol)
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
