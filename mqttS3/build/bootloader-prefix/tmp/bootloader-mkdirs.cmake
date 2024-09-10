# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/ESP32/esp-idf/v5.2.2/esp-idf/components/bootloader/subproject"
<<<<<<< HEAD
  "D:/ESP32/myProject/Esp32_node/mqttS3/build/bootloader"
  "D:/ESP32/myProject/Esp32_node/mqttS3/build/bootloader-prefix"
  "D:/ESP32/myProject/Esp32_node/mqttS3/build/bootloader-prefix/tmp"
  "D:/ESP32/myProject/Esp32_node/mqttS3/build/bootloader-prefix/src/bootloader-stamp"
  "D:/ESP32/myProject/Esp32_node/mqttS3/build/bootloader-prefix/src"
  "D:/ESP32/myProject/Esp32_node/mqttS3/build/bootloader-prefix/src/bootloader-stamp"
=======
  "D:/ESP32/myProject/uartnow/mqttS3/build/bootloader"
  "D:/ESP32/myProject/uartnow/mqttS3/build/bootloader-prefix"
  "D:/ESP32/myProject/uartnow/mqttS3/build/bootloader-prefix/tmp"
  "D:/ESP32/myProject/uartnow/mqttS3/build/bootloader-prefix/src/bootloader-stamp"
  "D:/ESP32/myProject/uartnow/mqttS3/build/bootloader-prefix/src"
  "D:/ESP32/myProject/uartnow/mqttS3/build/bootloader-prefix/src/bootloader-stamp"
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
<<<<<<< HEAD
    file(MAKE_DIRECTORY "D:/ESP32/myProject/Esp32_node/mqttS3/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/ESP32/myProject/Esp32_node/mqttS3/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
=======
    file(MAKE_DIRECTORY "D:/ESP32/myProject/uartnow/mqttS3/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/ESP32/myProject/uartnow/mqttS3/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
endif()
