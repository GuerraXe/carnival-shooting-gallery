# Collects the license texts of everything statically linked into the game,
# copied verbatim from the pinned dependency sources (never written by hand).
# Called from CMakeLists.txt at configure time; output goes to
# ${CMAKE_BINARY_DIR}/third-party-licenses/ and is installed with the game.

# Copies the text between `start` (inclusive) and the next `stop` (exclusive)
# out of a source file that embeds its license in a comment.
function(gallery_extract_license src start stop out)
    if(NOT EXISTS "${src}")
        message(FATAL_ERROR "License source not found: ${src}")
    endif()
    file(READ "${src}" content)
    string(FIND "${content}" "${start}" from)
    if(from EQUAL -1)
        message(FATAL_ERROR "License block '${start}' not found in ${src}")
    endif()
    string(SUBSTRING "${content}" ${from} -1 rest)
    string(FIND "${rest}" "${stop}" to)
    if(to EQUAL -1)
        message(FATAL_ERROR "End of license block not found in ${src}")
    endif()
    string(SUBSTRING "${rest}" 0 ${to} block)
    cmake_path(RELATIVE_PATH src BASE_DIRECTORY "${sfml_SOURCE_DIR}" OUTPUT_VARIABLE rel)
    file(WRITE "${out}"
        "Extracted verbatim from SFML 3.0.2 source: ${rel}\n\n${block}\n")
endfunction()

function(gallery_collect_third_party_licenses dest)
    file(MAKE_DIRECTORY "${dest}")
    foreach(dep IN ITEMS freetype ogg vorbis flac)
        FetchContent_GetProperties(${dep})
        if(NOT ${dep}_SOURCE_DIR)
            message(FATAL_ERROR "Dependency '${dep}' was not fetched; cannot collect its license")
        endif()
    endforeach()

    # Separate license files shipped by each project.
    configure_file("${sfml_SOURCE_DIR}/license.md"        "${dest}/SFML-license.md"          COPYONLY)
    configure_file("${freetype_SOURCE_DIR}/LICENSE.TXT"   "${dest}/FreeType-LICENSE.txt"     COPYONLY)
    configure_file("${freetype_SOURCE_DIR}/docs/FTL.TXT"  "${dest}/FreeType-FTL.txt"         COPYONLY)
    configure_file("${ogg_SOURCE_DIR}/COPYING"            "${dest}/libogg-COPYING.txt"       COPYONLY)
    configure_file("${vorbis_SOURCE_DIR}/COPYING"         "${dest}/libvorbis-COPYING.txt"    COPYONLY)
    configure_file("${flac_SOURCE_DIR}/COPYING.Xiph"      "${dest}/FLAC-COPYING.Xiph.txt"    COPYONLY)

    # Single-header libraries vendored inside SFML: license is in a comment.
    set(h "${sfml_SOURCE_DIR}/extlibs/headers")
    gallery_extract_license("${h}/miniaudio/miniaudio.h"
        "This software is available as a choice of the following licenses" "*/"
        "${dest}/miniaudio-LICENSE.txt")
    gallery_extract_license("${h}/stb_image/stb_image.h"
        "This software is available under 2 licenses" "*/"
        "${dest}/stb_image-LICENSE.txt")
    gallery_extract_license("${h}/minimp3/minimp3.h"
        "To the extent possible under law" "*/"
        "${dest}/minimp3-LICENSE.txt")
    gallery_extract_license("${h}/glad/include/glad/gl.h"
        "** Copyright (c) 2008-2018 The Khronos Group Inc." "*/"
        "${dest}/glad-khrplatform-LICENSE.txt")
endfunction()
