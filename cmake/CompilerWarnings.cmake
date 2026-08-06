# SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
# SPDX-License-Identifier: MIT

function(lsw_audio_diag_set_compiler_warnings target_name warnings_as_errors)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /Zc:__cplusplus /EHsc)
        if(warnings_as_errors)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Woverloaded-virtual)
        if(warnings_as_errors)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()
