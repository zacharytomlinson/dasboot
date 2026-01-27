# Prints a "flash-only" size view for dasboot by filtering avr-size output.

if (NOT DEFINED SIZE_TOOL OR SIZE_TOOL STREQUAL "")
    message(FATAL_ERROR "SIZE_TOOL is required (path to avr-size).")
endif ()

if (NOT DEFINED ELF_PATH OR ELF_PATH STREQUAL "")
    message(FATAL_ERROR "ELF_PATH is required (path to ELF file).")
endif ()

if (NOT DEFINED BOOT_SECTION_SIZE OR BOOT_SECTION_SIZE STREQUAL "")
    message(FATAL_ERROR "BOOT_SECTION_SIZE is required (e.g. 0x1000).")
endif ()

execute_process(
        COMMAND "${SIZE_TOOL}" -A "${ELF_PATH}"
        RESULT_VARIABLE _res
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err
)

if (NOT _res EQUAL 0)
    message(FATAL_ERROR "avr-size failed (${_res}): ${_err}")
endif ()

string(REPLACE "\r\n" "\n" _out "${_out}")
string(REPLACE "\r" "\n" _out "${_out}")
string(REPLACE "\n" ";" _lines "${_out}")

set(_size_text 0)
set(_size_data 0)
set(_size_bss 0)
set(_size_noinit 0)
set(_size_appvectors 0)
set(_addr_appvectors 0)
set(_size_eeprom 0)

foreach (_line IN LISTS _lines)
    if (_line MATCHES "^\\.(text|data|bss|noinit|appvectors|eeprom)[ \t]+([0-9]+)[ \t]+([0-9]+)")
        set(_sec "${CMAKE_MATCH_1}")
        set(_sz "${CMAKE_MATCH_2}")
        set(_addr "${CMAKE_MATCH_3}")

        if (_sec STREQUAL "text")
            set(_size_text "${_sz}")
        elseif (_sec STREQUAL "data")
            set(_size_data "${_sz}")
        elseif (_sec STREQUAL "bss")
            set(_size_bss "${_sz}")
        elseif (_sec STREQUAL "noinit")
            set(_size_noinit "${_sz}")
        elseif (_sec STREQUAL "appvectors")
            set(_size_appvectors "${_sz}")
            set(_addr_appvectors "${_addr}")
        elseif (_sec STREQUAL "eeprom")
            set(_size_eeprom "${_sz}")
        endif ()
    endif ()
endforeach ()

math(EXPR _boot_section_bytes "${BOOT_SECTION_SIZE}")
math(EXPR _boot_used "${_size_text} + ${_size_data}")
math(EXPR _boot_free "${_boot_section_bytes} - ${_boot_used}")
math(EXPR _sram_globals "${_size_data} + ${_size_bss} + ${_size_noinit}")

message(STATUS "dasboot size (flash-only view):")
message(STATUS "  BOOT (.text + .data): ${_boot_used} bytes (of ${_boot_section_bytes}) -> ${_boot_free} bytes free")
message(STATUS "  APPCODE vectors (.appvectors): ${_size_appvectors} bytes @ ${_addr_appvectors}")
message(STATUS "  SRAM globals (.data + .bss + .noinit): ${_sram_globals} bytes")
message(STATUS "  EEPROM (.eeprom): ${_size_eeprom} bytes")

