# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/naranjax/esp/esp-idf/components/bootloader/subproject"
  "/Users/naranjax/Downloads/ESP32-C6-Touch-LCD-1.47-Demo/ESP-IDF/03_lvgl_example/build/bootloader"
  "/Users/naranjax/Downloads/ESP32-C6-Touch-LCD-1.47-Demo/ESP-IDF/03_lvgl_example/build/bootloader-prefix"
  "/Users/naranjax/Downloads/ESP32-C6-Touch-LCD-1.47-Demo/ESP-IDF/03_lvgl_example/build/bootloader-prefix/tmp"
  "/Users/naranjax/Downloads/ESP32-C6-Touch-LCD-1.47-Demo/ESP-IDF/03_lvgl_example/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/naranjax/Downloads/ESP32-C6-Touch-LCD-1.47-Demo/ESP-IDF/03_lvgl_example/build/bootloader-prefix/src"
  "/Users/naranjax/Downloads/ESP32-C6-Touch-LCD-1.47-Demo/ESP-IDF/03_lvgl_example/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/naranjax/Downloads/ESP32-C6-Touch-LCD-1.47-Demo/ESP-IDF/03_lvgl_example/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/naranjax/Downloads/ESP32-C6-Touch-LCD-1.47-Demo/ESP-IDF/03_lvgl_example/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
