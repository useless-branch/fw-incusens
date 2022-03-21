cmake_minimum_required(VERSION 3.16)

project("Incusens Firmware (Incubator)" VERSION 0.1.0)

include(kvasir/cmake/kvasir.cmake)

add_subdirectory(recreated_goon)
add_subdirectory(aglio)

goon_generate(bootloader_commands kvasir/bootloader.idl cpp_command
    --type
    --member_list
    --command_variant
)

add_executable(development src/main.cpp)
target_configure_kvasir(development
    OPTIMIZATION_STRATEGY size
    USE_LOG
)
target_link_libraries(development bootloader_commands aglio)

add_executable(release src/main.cpp)
target_configure_kvasir(release
    OPTIMIZATION_STRATEGY size
    USE_LOG
    BOOTLOADER_SIZE 8192
    LINKER_FILE_TEMPLATE linker/app.ld.in
    BOOTLOADER bootloader
)
target_link_libraries(release bootloader_commands aglio)

add_executable(bootloader src/bootloader.cpp)
target_configure_kvasir(bootloader
    OPTIMIZATION_STRATEGY size
    LINKER_FILE_TEMPLATE linker/bootloader.ld.in
    ENABLE_SELF_OVERRIDE
    APPLICATION release
)
target_link_libraries(bootloader bootloader_commands aglio)
