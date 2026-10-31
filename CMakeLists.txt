cmake_minimum_required(VERSION 3.20)
project(Ja2Travnik LANGUAGES C CXX RC)

# ==============================================================================
# Зеркало решения ja2_VS2013.sln (Win32).
# ВАЖНО: проект 32-битный (Win32), прекомпилированные .lib (mss32, binkw32,
# RakNet и т.д.) собраны под x86. В CLion выберите 32-битную версию компилятора,
# например: -G "Visual Studio 17 2022" -A Win32
# ==============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Статическая рантайм-библиотека (/MT, /MTd) — как в vcxproj
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT WIN32)
    message(FATAL_ERROR "Этот проект собирается только под Windows (Win32)")
endif()

if(CMAKE_SIZEOF_VOID_P GREATER 4)
    message(WARNING "Сборка под x64! Прекомпилированные библиотеки 32-битные, линковка упадёт. Используйте тулчейн Win32 (например -A Win32).")
endif()

# ==============================================================================
# Препроцессорные определения (ja2.props + ja2_Debug.props + ja2_Editor.props)
# ==============================================================================
add_compile_definitions(
    WIN32 _WINDOWS _LIB
    JA2 CINTERFACE
    XML_STATIC
    VFS_STATIC VFS_WITH_SLF VFS_WITH_7ZIP USE_VFS
    _CRT_SECURE_NO_DEPRECATE _CRT_SECURE_NO_WARNINGS
    _CRT_NON_CONFORMING_SWPRINTFS _SCL_SECURE_NO_WARNINGS
    RUSSIAN
    LUA_BUILD_AS_DLL
)

if(MSVC)
    add_compile_options(/MP)          # многопроцессорная компиляция
    add_compile_options(/Zc:twoPhase-) # совместимость со старым кодом (без двухфазного поиска имён)

    # Определения Debug-конфигурации из ja2_Debug.props
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /JMC-")
endif()

# ==============================================================================
# Пути поиска заголовков (AdditionalIncludeDirectories из vcxproj-ов)
# ==============================================================================
set(JA2_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    "${JA2_ROOT}"
    "${JA2_ROOT}/Standard Gaming Platform"
    "${JA2_ROOT}/Utils"
    "${JA2_ROOT}/TileEngine"
    "${JA2_ROOT}/Tactical"
    "${JA2_ROOT}/TacticalAI"
    "${JA2_ROOT}/Tactical/LogicalBodyTypes"
    "${JA2_ROOT}/Strategic"
    "${JA2_ROOT}/Strategic/StrategicMilitiaCall"
    "${JA2_ROOT}/Laptop"
    "${JA2_ROOT}/Editor"
    "${JA2_ROOT}/Console"
    "${JA2_ROOT}/lua"
    "${JA2_ROOT}/Multiplayer"
    "${JA2_ROOT}/Multiplayer/raknet"
    "${JA2_ROOT}/ModularizedTacticalAI/include"
    "${JA2_ROOT}/Res"
    "${JA2_ROOT}/ext/VFS/include"
    "${JA2_ROOT}/ext/VFS/ext/7z/src"
    "${JA2_ROOT}/ext/VFS/ext/utfcpp/source"
    "${JA2_ROOT}/ext/libpng"
    "${JA2_ROOT}/ext/zlib"
)

# ==============================================================================
# Сторонние статические библиотеки
# ==============================================================================

# --- zlib (ext/libpng/projects/visualc71/zlib_VS2013.vcxproj) ---
add_library(Sub_ZLib STATIC
    "ext/zlib/adler32.c" "ext/zlib/compress.c" "ext/zlib/crc32.c" "ext/zlib/deflate.c"
    "ext/zlib/gzclose.c" "ext/zlib/gzread.c" "ext/zlib/gzwrite.c" "ext/zlib/infback.c"
    "ext/zlib/inffast.c" "ext/zlib/inflate.c" "ext/zlib/inftrees.c" "ext/zlib/trees.c"
    "ext/zlib/uncompr.c" "ext/zlib/zutil.c"
)

# --- libpng (ext/libpng/projects/visualc71/libpng_VS2013.vcxproj) ---
add_library(Sub_LibPNG STATIC
    "ext/libpng/png.c" "ext/libpng/pngerror.c" "ext/libpng/pnggccrd.c" "ext/libpng/pngget.c"
    "ext/libpng/pngmem.c" "ext/libpng/pngpread.c" "ext/libpng/pngread.c" "ext/libpng/pngrio.c"
    "ext/libpng/pngrtran.c" "ext/libpng/pngrutil.c" "ext/libpng/pngset.c" "ext/libpng/pngtrans.c"
    "ext/libpng/pngwio.c" "ext/libpng/pngwrite.c" "ext/libpng/pngwtran.c" "ext/libpng/pngwutil.c"
)
target_compile_definitions(Sub_LibPNG PRIVATE PNG_NO_MMX_CODE)
target_include_directories(Sub_LibPNG PRIVATE "${JA2_ROOT}/ext/zlib")

# --- 7z (ext/VFS/build/7z_VS2013.vcxproj, исходники в ext/VFS/ext/7z/src) ---
add_library(Sub_7z STATIC
    "ext/VFS/ext/7z/src/7zAlloc.c" "ext/VFS/ext/7z/src/7zBuf.c" "ext/VFS/ext/7z/src/7zBuf2.c"
    "ext/VFS/ext/7z/src/7zCrc.c" "ext/VFS/ext/7z/src/7zCrcOpt.c" "ext/VFS/ext/7z/src/7zDec.c"
    "ext/VFS/ext/7z/src/7zFile.c" "ext/VFS/ext/7z/src/7zIn.c" "ext/VFS/ext/7z/src/7zStream.c"
    "ext/VFS/ext/7z/src/Alloc.c" "ext/VFS/ext/7z/src/Bcj2.c" "ext/VFS/ext/7z/src/Bra.c"
    "ext/VFS/ext/7z/src/Bra86.c" "ext/VFS/ext/7z/src/BraIA64.c" "ext/VFS/ext/7z/src/CpuArch.c"
    "ext/VFS/ext/7z/src/Delta.c" "ext/VFS/ext/7z/src/LzFind.c" "ext/VFS/ext/7z/src/LzFindMt.c"
    "ext/VFS/ext/7z/src/Lzma2Dec.c" "ext/VFS/ext/7z/src/Lzma2Enc.c" "ext/VFS/ext/7z/src/Lzma86Dec.c"
    "ext/VFS/ext/7z/src/Lzma86Enc.c" "ext/VFS/ext/7z/src/LzmaDec.c" "ext/VFS/ext/7z/src/LzmaEnc.c"
    "ext/VFS/ext/7z/src/LzmaLib.c" "ext/VFS/ext/7z/src/MtCoder.c" "ext/VFS/ext/7z/src/Ppmd7.c"
    "ext/VFS/ext/7z/src/Ppmd7Dec.c" "ext/VFS/ext/7z/src/Ppmd7Enc.c" "ext/VFS/ext/7z/src/Sha256.c"
    "ext/VFS/ext/7z/src/Threads.c" "ext/VFS/ext/7z/src/Xz.c" "ext/VFS/ext/7z/src/XzCrc64.c"
    "ext/VFS/ext/7z/src/XzDec.c" "ext/VFS/ext/7z/src/XzEnc.c" "ext/VFS/ext/7z/src/XzIn.c"
)
target_compile_definitions(Sub_7z PRIVATE SZIP_STATIC)

# --- bfVFS (ext/VFS/build/VFS_VS2013.vcxproj) ---
add_library(Sub_VFS STATIC
    "ext/VFS/src/Aspects/vfs_logging.cpp" "ext/VFS/src/Aspects/vfs_settings.cpp"
    "ext/VFS/src/Aspects/vfs_synchronization.cpp"
    "ext/VFS/src/Core/File/vfs_buffer_file.cpp" "ext/VFS/src/Core/File/vfs_dir_file.cpp"
    "ext/VFS/src/Core/File/vfs_file.cpp" "ext/VFS/src/Core/File/vfs_lib_file.cpp"
    "ext/VFS/src/Core/Interface/vfs_interface_members.cpp"
    "ext/VFS/src/Core/Location/vfs_directory_tree.cpp" "ext/VFS/src/Core/Location/vfs_lib_dir.cpp"
    "ext/VFS/src/Core/Location/vfs_uncompressed_lib_base.cpp"
    "ext/VFS/src/Core/vfs.cpp" "ext/VFS/src/Core/vfs_debug.cpp" "ext/VFS/src/Core/vfs_file_raii.cpp"
    "ext/VFS/src/Core/vfs_init.cpp" "ext/VFS/src/Core/vfs_os_functions.cpp"
    "ext/VFS/src/Core/vfs_path.cpp" "ext/VFS/src/Core/vfs_profile.cpp" "ext/VFS/src/Core/vfs_string.cpp"
    "ext/VFS/src/Core/vfs_types.cpp" "ext/VFS/src/Core/vfs_vfile.cpp" "ext/VFS/src/Core/vfs_vloc.cpp"
    "ext/VFS/src/Ext/7z/vfs_7z_library.cpp" "ext/VFS/src/Ext/7z/vfs_create_7z_library.cpp"
    "ext/VFS/src/Ext/slf/vfs_slf_library.cpp"
    "ext/VFS/src/Tools/vfs_allocator.cpp" "ext/VFS/src/Tools/vfs_file_logger.cpp"
    "ext/VFS/src/Tools/vfs_hp_timer.cpp" "ext/VFS/src/Tools/vfs_log.cpp"
    "ext/VFS/src/Tools/vfs_parser_tools.cpp" "ext/VFS/src/Tools/vfs_profiler.cpp"
    "ext/VFS/src/Tools/vfs_property_container.cpp" "ext/VFS/src/Tools/vfs_tools.cpp"
)
target_compile_definitions(Sub_VFS PRIVATE SZIP_STATIC)

# ==============================================================================
# Статические библиотеки игры (все проекты из ja2_VS2013.sln)
# ==============================================================================

# --- SGP (Standard Gaming Platform/SGP_VS2013.vcxproj) ---
file(GLOB SGP_SOURCES "Standard Gaming Platform/*.cpp")
add_library(Sub_SGP STATIC ${SGP_SOURCES})

# --- Utils (Utils/Utils_VS2013.vcxproj) ---
file(GLOB UTILS_SOURCES "Utils/*.cpp")
add_library(Sub_Utils STATIC ${UTILS_SOURCES})

# --- TileEngine (TileEngine/TileEngine_VS2013.vcxproj) ---
file(GLOB TILEENGINE_SOURCES "TileEngine/*.cpp")
add_library(Sub_TileEngine STATIC ${TILEENGINE_SOURCES})

# --- TacticalAI (TacticalAI/TacticalAI_VS2013.vcxproj) ---
file(GLOB TACTICALAI_SOURCES "TacticalAI/*.cpp")
add_library(Sub_TacticalAI STATIC ${TACTICALAI_SOURCES})

# --- Tactical (Tactical/Tactical_VS2013.vcxproj, включает LogicalBodyTypes) ---
file(GLOB TACTICAL_SOURCES "Tactical/*.cpp" "Tactical/LogicalBodyTypes/*.cpp")
list(REMOVE_ITEM TACTICAL_SOURCES "${JA2_ROOT}/Tactical/AStarPathfinder.cpp") # отсутствует в vcxproj
add_library(Sub_Tactical STATIC ${TACTICAL_SOURCES})

# --- Strategic (Strategic/Strategic_VS2013.vcxproj, включает StrategicMilitiaCall) ---
file(GLOB STRATEGIC_SOURCES "Strategic/*.cpp" "Strategic/StrategicMilitiaCall/*.cpp")
add_library(Sub_Strategic STATIC ${STRATEGIC_SOURCES})

# --- Laptop (Laptop/Laptop_VS2013.vcxproj) ---
file(GLOB LAPTOP_SOURCES "Laptop/*.cpp")
add_library(Sub_Laptop STATIC ${LAPTOP_SOURCES})

# --- Editor (Editor/Editor_VS2013.vcxproj) ---
add_library(Sub_Editor STATIC
    "Editor/Cursor Modes.cpp" "Editor/Editor Callbacks.cpp" "Editor/Editor Modes.cpp"
    "Editor/Editor Taskbar Creation.cpp" "Editor/Editor Taskbar Utils.cpp" "Editor/Editor Undo.cpp"
    "Editor/EditorBuildings.cpp" "Editor/EditorItems.cpp" "Editor/EditorMapInfo.cpp"
    "Editor/EditorMercs.cpp" "Editor/EditorTerrain.cpp" "Editor/editscreen.cpp"
    "Editor/edit_sys.cpp" "Editor/Item Statistics.cpp" "Editor/LoadScreen.cpp"
    "Editor/messagebox.cpp" "Editor/newsmooth.cpp" "Editor/popupmenu.cpp"
    "Editor/Road Smoothing.cpp" "Editor/Sector Summary.cpp" "Editor/selectwin.cpp"
    "Editor/SmartMethod.cpp" "Editor/smooth.cpp" "Editor/Smoothing Utils.cpp"
    "Editor/XML_ActionItems.cpp"
)

# --- Console (Console/Console_VS2013.vcxproj) ---
add_library(Sub_Console STATIC
    "Console/Console.cpp" "Console/Cursors.cpp" "Console/Dialogs.cpp" "Console/FileStream.cpp"
)

# --- lua (lua/lua_VS2013.vcxproj, линкуется с прекомпилированной lua51.lib) ---
add_library(Sub_Lua STATIC
    "lua/lua.cpp" "lua/lua_class_interface.cpp" "lua/lua_env.cpp" "lua/lua_function.cpp"
    "lua/lua_state.cpp" "lua/lua_strategic.cpp" "lua/lua_table.cpp" "lua/lua_tactical.cpp"
    "lua/lwstring.cpp"
)

# --- ModularizedTacticalAI (ModularizedTacticalAI/ModularizedTacticalAI_VS2013.vcxproj) ---
add_library(Sub_ModularizedTacticalAI STATIC
    "ModularizedTacticalAI/src/AbstractPlanFactory.cpp" "ModularizedTacticalAI/src/CrowPlan.cpp"
    "ModularizedTacticalAI/src/LegacyAIPlan.cpp" "ModularizedTacticalAI/src/LegacyAIPlanFactory.cpp"
    "ModularizedTacticalAI/src/LegacyCreaturePlan.cpp" "ModularizedTacticalAI/src/LegacyZombiePlan.cpp"
    "ModularizedTacticalAI/src/NullPlan.cpp" "ModularizedTacticalAI/src/NullPlanFactory.cpp"
    "ModularizedTacticalAI/src/Plan.cpp" "ModularizedTacticalAI/src/PlanFactoryLibrary.cpp"
    "ModularizedTacticalAI/src/PlanList.cpp"
)
target_include_directories(Sub_ModularizedTacticalAI PUBLIC "${JA2_ROOT}/ModularizedTacticalAI/include")

# ==============================================================================
# Главный исполняемый файл (ja2_VS2013.vcxproj)
# ==============================================================================
add_executable(Ja2_travnik WIN32
    "aniviewscreen.cpp" "Credits.cpp" "Fade Screen.cpp" "GameInitOptionsScreen.cpp"
    "gameloop.cpp" "gamescreen.cpp" "GameSettings.cpp" "GameVersion.cpp" "HelpScreen.cpp"
    "Init.cpp" "Intro.cpp" "JA2 Splash.cpp" "Ja25Update.cpp" "jascreens.cpp"
    "Language Defines.cpp" "Loading Screen.cpp" "MainMenuScreen.cpp" "MessageBoxScreen.cpp"
    "MPChatScreen.cpp" "MPConnectScreen.cpp" "MPHostScreen.cpp" "MPJoinScreen.cpp"
    "MPScoreScreen.cpp" "MPXmlTeams.cpp" "Options Screen.cpp" "profiler.cpp"
    "SaveLoadGame.cpp" "SaveLoadScreen.cpp" "Screens.cpp" "Sys Globals.cpp" "ub_config.cpp"
    "XML_IntroFiles.cpp" "XML_Layout_MainMenu.cpp"
    "Multiplayer/client.cpp" "Multiplayer/server.cpp" "Multiplayer/transfer_rules.cpp"
    "Res/ja2.rc" "Console/Console.rc"
)

set_target_properties(Ja2_travnik PROPERTIES
    OUTPUT_NAME "Ja2+AI_travnik_edition"
    RUNTIME_OUTPUT_DIRECTORY "${JA2_ROOT}/../JA2_Game"
)

# ==============================================================================
# Линковка (ja2_Libs.props + ja2_LinkerVS2013.props)
# ==============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RAKNET_LIB "${JA2_ROOT}/Multiplayer/raknet/RakNetLibStaticDebug.lib")
else()
    set(RAKNET_LIB "${JA2_ROOT}/Multiplayer/raknet/RakNetLibStatic.lib")
endif()

target_link_libraries(Ja2_travnik PRIVATE
    Sub_Console Sub_Editor Sub_Laptop Sub_Lua Sub_SGP Sub_Strategic Sub_Tactical
    Sub_TacticalAI Sub_TileEngine Sub_Utils
    Sub_ModularizedTacticalAI
    Sub_VFS Sub_7z Sub_LibPNG Sub_ZLib

    # Системные библиотеки (AdditionalDependencies из ja2_VS2013.vcxproj)
    Winmm ws2_32 DbgHelp
    user32 gdi32 advapi32 shell32 ole32 uuid kernel32

    # Прекомпилированные библиотеки (ja2_Libs.props)
    "${JA2_ROOT}/libexpatMT.lib"
    "${JA2_ROOT}/mss32.lib"
    "${JA2_ROOT}/SMACKW32.LIB"
    "${JA2_ROOT}/binkw32.lib"
    "${JA2_ROOT}/fmodvc.lib"
    "${JA2_ROOT}/lua51.lib"
    "${JA2_ROOT}/VtuneApi.lib"
    "${JA2_ROOT}/Standard Gaming Platform/ddraw.lib"
    "${RAKNET_LIB}"
)

# Опции линкера из vcxproj (RandomizedBaseAddress=false, ImageHasSafeExceptionHandlers=false)
if(MSVC)
    target_link_options(Ja2_travnik PRIVATE /SAFESEH:NO /DYNAMICBASE:NO)
endif()
