# Minimal AVR-GCC toolchain file for CMake.
#
# Usage:
#   cmake -S . -B build-fw -DCMAKE_TOOLCHAIN_FILE=cmake/avr-gcc-toolchain.cmake
#   cmake --build build-fw

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_ASM_COMPILER avr-gcc)

find_program(CMAKE_OBJCOPY avr-objcopy REQUIRED)
find_program(CMAKE_OBJDUMP avr-objdump REQUIRED)
find_program(CMAKE_SIZE avr-size REQUIRED)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(MCU "atmega4808" CACHE STRING "AVR MCU type (avr-gcc -mmcu=...)")
set(F_CPU "20000000UL" CACHE STRING "CPU frequency (Hz)")

set(AVR_MCU_FLAGS "-mmcu=${MCU}")

set(CMAKE_C_FLAGS_INIT "${AVR_MCU_FLAGS} -DF_CPU=${F_CPU} -Os -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "${AVR_MCU_FLAGS} -DF_CPU=${F_CPU} -Os -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS_INIT "${AVR_MCU_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "${AVR_MCU_FLAGS} -Wl,--gc-sections")
