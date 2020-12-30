# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

list(APPEND EXTRA_COMPONENT_DIRS components/ds3231 components/open-authenticator-component components/lv_port_esp32/components/lv_examples components/lv_port_esp32/components/lvgl components/lv_port_esp32/components/lvgl_esp32_drivers/lvgl_tft components/lv_port_esp32/components/lvgl_esp32_drivers/lvgl_touch components/lv_port_esp32/components/lvgl_esp32_drivers)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(open-authenticator-app)
