cmake_minimum_required(VERSION 3.16)

project("Incusens_Firmware" VERSION 0.1.0)

include(kvasir/cmake/kvasir.cmake)

add_executable(development src/main.cpp)
target_configure_kvasir(development
    OPTIMIZATION_STRATEGY size
    USE_LOG
)
