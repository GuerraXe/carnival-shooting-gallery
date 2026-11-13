# Carnival Shooting Gallery — build definition.
# See docs/ARCHITECTURE.md §8 and docs/BUILD.md for the rationale and commands.

cmake_minimum_required(VERSION 3.26)   # 3.26: `copy_directory_if_different`

project(CarnivalShootingGallery
    VERSION 0.1.0
    DESCRIPTION "First-person carnival duck shooting gallery"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Preview distribution (D-033): Release-optimized, but allowed to run with the
# labelled stand-ins while MVP photographs are missing, and titled "(Preview)".
# OFF = the strict Release build guarded by the D-023 asset gate. Set by the
# `windows-msvc-preview` configure preset; see docs/BUILD.md "Distribution".
option(GALLERY_PREVIEW "Build the stand-in-tolerant preview distribution" OFF)
if(GALLERY_PREVIEW)
    set(GALLERY_VERSION_STRING "${PROJECT_VERSION}-preview")
else()
    set(GALLERY_VERSION_STRING "${PROJECT_VERSION}")
endif()

# ---------------------------------------------------------------------------
# Dependencies (D-002, D-003): SFML 3, pinned, static.
# ---------------------------------------------------------------------------
include(FetchContent)

# Static SFML so the game ships as one exe + assets/ (D-003).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)

# SFML 3.0.2 — tag 3.0.2, commit 0fa201c969e48ecc253581c5841ce73f44d42f49.
# Fetched as the GitHub archive of that exact commit and verified by SHA-256,
# so the source cannot change underneath us. SFML 3.0.2 in turn pins its own
# sub-dependencies to release tags: FreeType VER-2-13-2, ogg v1.3.5,
# vorbis v1.3.7, FLAC 1.4.3 (miniaudio is vendored inside SFML).
#
# Offline / TLS-interception fallback (risk R-4): download the archive by hand,
# extract it, and configure with
#   -DFETCHCONTENT_SOURCE_DIR_SFML=<path to extracted SFML-0fa201c…>
FetchContent_Declare(SFML
    URL      https://github.com/SFML/SFML/archive/0fa201c969e48ecc253581c5841ce73f44d42f49.tar.gz
    URL_HASH SHA256=df03afdc2858d0e15e1c9ff06bddd113c18b2a6c089532f5b8d05dbb2e653f2a
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SYSTEM            # SFML headers are external: our /WX must not apply to them
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(SFML)

# License texts of everything linked into the exe, for the distribution.
include(cmake/ThirdPartyNotices.cmake)
gallery_collect_third_party_licenses("${CMAKE_BINARY_DIR}/third-party-licenses")

# ---------------------------------------------------------------------------
# Project-wide warning policy (D-001): strict on OUR targets only.
# ---------------------------------------------------------------------------
add_library(gallery_warnings INTERFACE)
if(MSVC)
    target_compile_options(gallery_warnings INTERFACE /W4 /WX /permissive- /utf-8 /external:W0)
else()
    target_compile_options(gallery_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

# ---------------------------------------------------------------------------
# gallery_logic — pure, SFML-free library (D-005).
# It deliberately does NOT link SFML: an SFML include here fails to compile.
# ---------------------------------------------------------------------------
add_library(gallery_logic STATIC
    src/logic/audio/Soundscape.cpp
    src/logic/core/FrameStats.cpp
    src/logic/core/Time.cpp
    src/logic/flow/Game.cpp
    src/logic/flow/UiLayout.cpp
    src/logic/game/AlphaMask.cpp
    src/logic/game/DuckConfig.cpp
    src/logic/game/DuckLane.cpp
    src/logic/game/DuckMotion.cpp
    src/logic/game/Occluder.cpp
    src/logic/game/RoundClock.cpp
    src/logic/game/Shooting.cpp
    src/logic/view/Placement.cpp
    src/logic/view/SceneLayout.cpp
    src/logic/view/Viewport.cpp)
target_include_directories(gallery_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(gallery_logic PRIVATE gallery_warnings)

# ---------------------------------------------------------------------------
# carnival_gallery — the SFML shell executable.
# ---------------------------------------------------------------------------
add_executable(carnival_gallery
    src/main.cpp
    src/shell/app/Application.cpp
    src/shell/app/LaunchOptions.cpp
    src/shell/assets/AssetPaths.cpp
    src/shell/assets/AssetStore.cpp
    src/shell/audio/AudioSystem.cpp
    src/shell/platform/Logger.cpp
    src/shell/platform/Platform.cpp
    src/shell/platform/Input.cpp
    src/shell/platform/Window.cpp
    src/shell/render/DrawUtil.cpp
    src/shell/render/DuckVisuals.cpp
    src/shell/render/PostProcess.cpp
    src/shell/render/Renderer.cpp
    src/shell/render/Screens.cpp
    src/shell/render/StandIn.cpp
    src/shell/render/Theme.cpp
    src/shell/platform/carnival_gallery.manifest    # per-monitor-v2 DPI awareness (R-6)
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc)          # Explorer Properties > Details
if(GALLERY_PREVIEW)
    set(GALLERY_RC_FILEFLAGS "VS_FF_PRERELEASE")
else()
    set(GALLERY_RC_FILEFLAGS "0")
endif()
configure_file(src/shell/platform/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
target_include_directories(carnival_gallery PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(carnival_gallery PRIVATE
    gallery_logic
    gallery_warnings
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Main)   # provides WinMain -> main() for the WIN32 subsystem build
target_compile_definitions(carnival_gallery PRIVATE
    GALLERY_VERSION="${GALLERY_VERSION_STRING}"
    GALLERY_PREVIEW=$<BOOL:${GALLERY_PREVIEW}>
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

# Console subsystem in Debug (visible logs), WIN32 subsystem otherwise.
# The CMake target keeps its short name; the shipped file is the product name.
set_target_properties(carnival_gallery PROPERTIES
    OUTPUT_NAME CarnivalShootingGallery
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:carnival_gallery>)

# Assets are located relative to the executable, never the working directory.
# Mirror assets/ next to the exe after every build so the build directory is
# always launchable. (Files deleted from assets/ are not removed from the copy;
# delete <build>/<config>/assets to force a clean mirror.)
add_custom_command(TARGET carnival_gallery POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:carnival_gallery>/assets
    COMMENT "Mirroring assets/ next to carnival_gallery")

# ---------------------------------------------------------------------------
# audio_prep — development tool that cuts runtime sounds from the verified
# originals in asset-src/ (D-030). Never shipped; not part of the default
# build. Build with:  cmake --build --preset debug --target audio_prep
# ---------------------------------------------------------------------------
add_executable(audio_prep EXCLUDE_FROM_ALL tools/audio_prep.cpp)
target_link_libraries(audio_prep PRIVATE gallery_warnings SFML::Audio)

# sprite_bake — development tool that saves the game's grey shapes
# (src/shell/render/StandIn.cpp) as the sprite files under assets/images/
# (D-044). Never shipped. Build and run with:
#   cmake --build --preset debug --target sprite_bake
#   .\build\windows-msvc\Debug\sprite_bake.exe assets
add_executable(sprite_bake EXCLUDE_FROM_ALL
    tools/sprite_bake.cpp
    src/shell/render/DrawUtil.cpp
    src/shell/render/StandIn.cpp
    src/shell/render/Theme.cpp)
target_include_directories(sprite_bake PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(sprite_bake PRIVATE gallery_logic gallery_warnings SFML::Graphics)

# ---------------------------------------------------------------------------
# Tests — logic library only, hand-rolled framework (D-017).
# ---------------------------------------------------------------------------
enable_testing()
add_executable(gallery_tests
    tests/test_main.cpp
    tests/alpha_mask_tests.cpp
    tests/duck_lane_tests.cpp
    tests/duck_motion_tests.cpp
    tests/game_flow_tests.cpp
    tests/round_clock_tests.cpp
    tests/soundscape_tests.cpp
    tests/atmosphere_tests.cpp
    tests/shooting_tests.cpp
    tests/time_tests.cpp
    tests/placement_tests.cpp
    tests/scene_layout_tests.cpp
    tests/viewport_tests.cpp
    tests/verification_tests.cpp
    tests/arcade_tests.cpp)
target_include_directories(gallery_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(gallery_tests PRIVATE gallery_logic gallery_warnings)
add_test(NAME gallery_tests COMMAND gallery_tests)

# Asset contract (ARCHITECTURE §7.1, D-012, D-023): catalog vs
# assets/ASSET_MANIFEST.md vs files on disk. In the Release configuration the
# release gate is ON: any MVP asset not READY, or any TEMP_ file, fails it.
add_executable(asset_contract tests/asset_contract.cpp)
target_include_directories(asset_contract PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(asset_contract PRIVATE gallery_logic gallery_warnings)
add_test(NAME asset_contract
         COMMAND asset_contract ${CMAKE_CURRENT_SOURCE_DIR}
                 $<$<AND:$<CONFIG:Release>,$<NOT:$<BOOL:${GALLERY_PREVIEW}>>>:--release-gate>)

# ---------------------------------------------------------------------------
# Distribution (D-033): `cmake --install` / `cpack` lay out a self-contained
# folder: the exe, its runtime assets, and every license it needs. Nothing
# outside that folder is read at run time except Windows system fonts (TEMP
# display-font fallback, see README "Known limitations").
# ---------------------------------------------------------------------------
# Everything is in component "Game" so SFML's own install rules (headers,
# static libs) never reach the package. Install with --component Game.
install(TARGETS carnival_gallery RUNTIME DESTINATION . COMPONENT Game)
install(DIRECTORY assets/ DESTINATION assets COMPONENT Game
        PATTERN ".gitkeep" EXCLUDE)
install(FILES README.md THIRD_PARTY_NOTICES.md DESTINATION . COMPONENT Game)
install(DIRECTORY ${CMAKE_BINARY_DIR}/third-party-licenses/
        DESTINATION licenses/third-party COMPONENT Game)

# Ready-to-play copy: every preview build also lays out the same folder as the
# zip at <project>/game/, so the exe to double-click is always
# game/CarnivalShootingGallery.exe. Generated (gitignored); delete it to clear
# out files that were removed from assets/.
if(GALLERY_PREVIEW)
    add_custom_command(TARGET carnival_gallery POST_BUILD
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>
                --component Game --prefix ${CMAKE_CURRENT_SOURCE_DIR}/game
        COMMENT "Installing the playable game to game/")
endif()

set(CPACK_PACKAGE_NAME "CarnivalShootingGallery")
set(CPACK_PACKAGE_VENDOR "Carnival Shooting Gallery project")
set(CPACK_PACKAGE_VERSION "${GALLERY_VERSION_STRING}")
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "CarnivalShootingGallery-${GALLERY_VERSION_STRING}-win64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_VERBATIM_VARIABLES ON)
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};Game;/")
include(CPack)
