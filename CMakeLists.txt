cmake_minimum_required(VERSION 3.7)
project(iaq)

set(CMAKE_CXX_STANDARD 14)

link_libraries(paho-mqtt3c)
add_executable(iaq main.cpp CCS811.cpp CCS811.h SI7021.cpp SI7021.h BMP280.cpp BMP280.h)
