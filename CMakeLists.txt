cmake_minimum_required(VERSION 3.16)

project(dasboot LANGUAGES C CXX ASM)

if (NOT CMAKE_C_COMPILER MATCHES "avr-gcc")
    message(WARNING
            "This project is intended to be configured with avr-gcc. "
            "Example: cmake -S . -B build-fw -DCMAKE_TOOLCHAIN_FILE=cmake/avr-gcc-toolchain.cmake"
    )
endif ()

set(MCU "atmega4808" CACHE STRING "AVR MCU type (avr-gcc -mmcu=...)")
set(F_CPU "20000000UL" CACHE STRING "CPU frequency for firmware (Hz)")
set(BOOT_SECTION_SIZE "0x1000" CACHE STRING "Boot section size in bytes (must match BOOTEND fuse)")
set(APP_TRAMPOLINE_SIZE "0x0100" CACHE STRING "Bytes reserved for APPCODE vector trampoline")

if (NOT CMAKE_OBJCOPY)
    find_program(CMAKE_OBJCOPY avr-objcopy REQUIRED)
endif ()

if (NOT CMAKE_SIZE)
    find_program(CMAKE_SIZE avr-size REQUIRED)
endif ()

add_executable(dasboot
        src/main.c
        src/app_trampoline.S
        src/boot_eeprom.c
        src/boot_eeprom.h
        src/boot_contract.h
        src/slots.c
        src/slots.h
        src/nvm_flash.c
        src/nvm_flash.h
        src/board.h
        src/mcu.c
        src/mcu.h
        src/gpio.h
        src/status_led.c
        src/status_led.h
        src/twi0.c
        src/twi0.h
        src/spi.c
        src/spi.h
        src/drivers/w25q.c
        src/drivers/w25q.h
        src/drivers/atecc608a_bl.c
        src/drivers/atecc608a_bl.h
)

target_compile_features(dasboot PRIVATE c_std_11)
target_compile_options(dasboot PRIVATE -Wall -Wextra -Wundef -Os -mcall-prologues)

target_compile_definitions(dasboot PRIVATE
        F_CPU=${F_CPU}
        BOOT_SECTION_SIZE=${BOOT_SECTION_SIZE}
        APP_TRAMPOLINE_SIZE=${APP_TRAMPOLINE_SIZE}
)

target_include_directories(dasboot PRIVATE src)

# Place the APPCODE trampoline vectors exactly at the start of APPCODE (BOOT_SECTION_SIZE).
# Ensure the section is retained even with --gc-sections.
target_link_options(dasboot PRIVATE
        -Wl,--relax
        -Wl,--section-start=.appvectors=${BOOT_SECTION_SIZE}
        -Wl,--undefined=__app_trampoline_vectors
)

# Post-build artifacts
add_custom_command(TARGET dasboot POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom $<TARGET_FILE:dasboot> ${CMAKE_CURRENT_BINARY_DIR}/dasboot.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary -R .eeprom $<TARGET_FILE:dasboot> ${CMAKE_CURRENT_BINARY_DIR}/dasboot.bin
        BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/dasboot.hex
        ${CMAKE_CURRENT_BINARY_DIR}/dasboot.bin
        COMMENT "Generating .hex and .bin"
)

add_custom_target(size
        COMMAND ${CMAKE_SIZE} -C --mcu=${MCU} $<TARGET_FILE:dasboot>
        DEPENDS dasboot
        COMMENT "avr-size for dasboot"
        USES_TERMINAL
)

add_custom_target(size_sections
        COMMAND "${CMAKE_COMMAND}"
        -DSIZE_TOOL="${CMAKE_SIZE}"
        -DELF_PATH="$<TARGET_FILE:dasboot>"
        -DBOOT_SECTION_SIZE="${BOOT_SECTION_SIZE}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/print_size_sections.cmake"
        DEPENDS dasboot
        COMMENT "Flash-only size summary for dasboot"
        USES_TERMINAL
)

# One-time ATECC provisioning firmware (optional).
add_executable(atecc_provision
        src/provision_atecc.c
        src/board.h
        src/mcu.c
        src/mcu.h
        src/gpio.h
        src/status_led.c
        src/status_led.h
        src/twi0.c
        src/twi0.h
        src/drivers/atecc608a.c
        src/drivers/atecc608a.h
)
target_compile_features(atecc_provision PRIVATE c_std_11)
target_compile_options(atecc_provision PRIVATE -Wall -Wextra -Wundef -Os)
target_compile_definitions(atecc_provision PRIVATE F_CPU=${F_CPU})
target_include_directories(atecc_provision PRIVATE src)

add_custom_command(TARGET atecc_provision POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom $<TARGET_FILE:atecc_provision>
        ${CMAKE_CURRENT_BINARY_DIR}/atecc_provision.hex
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/atecc_provision.hex
        COMMENT "Generating atecc_provision.hex"
)

# Optional convenience target for flashing via UPDI (requires hardware)
set(AVRDUDE_PORT "" CACHE STRING "Serial port for avrdude (e.g. /dev/ttyUSB0)")
set(AVRDUDE_PROGRAMMER "serialupdi" CACHE STRING "avrdude programmer type")
add_custom_target(flash
        COMMAND avrdude -p m4808 -c ${AVRDUDE_PROGRAMMER} -P ${AVRDUDE_PORT} -U flash:w:${CMAKE_CURRENT_BINARY_DIR}/dasboot.hex:i
        DEPENDS dasboot
        USES_TERMINAL
)
