# Ice Pulse Sensor Firmware
cmake_minimum_required(VERSION 3.16)

# Set project name
set(PROJECT_NAME "ice-pulse-sensor")

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set component directories
set(EXTRA_COMPONENT_DIRS "components")

# Project configuration
project(${PROJECT_NAME})