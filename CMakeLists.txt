cmake_minimum_required(VERSION 3.25)
project(OSCMessenger)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SHARED_MODULE_PREFIX "")

if(APPLE OR WIN32)
    set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

# Windows - puts redistributable DLLs in install directory
include(InstallRequiredSystemLibraries)

# optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_COMPILER_IS_CLANG 1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
    add_definitions(-fvisibility=hidden)

    include (CheckCCompilerFlag)
    include (CheckCXXCompilerFlag)

    CHECK_C_COMPILER_FLAG(-msse HAS_SSE)
    CHECK_CXX_COMPILER_FLAG(-msse HAS_CXX_SSE)

    if (HAS_SSE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse")
    endif()
    if (HAS_CXX_SSE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse")
    endif()

    CHECK_C_COMPILER_FLAG(-msse2 HAS_SSE2)
    CHECK_CXX_COMPILER_FLAG(-msse2 HAS_CXX_SSE2)

    if (HAS_SSE2)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
    endif()
    if (HAS_CXX_SSE2)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    endif()

    CHECK_C_COMPILER_FLAG(-mfpmath=sse HAS_FPMATH_SSE)
    CHECK_CXX_COMPILER_FLAG(-mfpmath=sse HAS_CXX_FPMATH_SSE)

    if (HAS_FPMATH_SSE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse")
    endif()
    if (HAS_CXX_FPMATH_SSE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse")
    endif()

    if(NATIVE)
        add_definitions(-march=native)
    endif()

    if(CPP11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        if(CMAKE_COMPILER_IS_CLANG)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        endif()
    endif()
endif()

add_library(OSCMessenger MODULE src/OSCMessenger.cpp)

target_include_directories(OSCMessenger PUBLIC ${SC_SRC_PATH}/include/plugin_interface)
target_include_directories(OSCMessenger PUBLIC ${SC_SRC_PATH}/include/common)
target_include_directories(OSCMessenger PUBLIC ${SC_SRC_PATH}/common)

# external libs
target_include_directories(OSCMessenger PUBLIC ${PROJECT_SOURCE_DIR}/external/oscpp/include)
target_include_directories(OSCMessenger PUBLIC ${PROJECT_SOURCE_DIR}/external/asio/asio/include)

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)
endif()

# windows fix from https://github.com/supercollider/supercollider/blob/develop/tools/cmake_gen/SuperColliderCompilerConfig.cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # these options only apply if we're doing a 32-bit build, otherwise they cause a diagnostic
    # https://stackoverflow.com/questions/1067630/sse2-option-in-visual-c-x64
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_options(${target} PUBLIC
            $<$<BOOL:${has_sse}>:/arch:SSE>
            $<$<BOOL:${has_sse2}>:/arch:SSE2>
            )
    endif()
    if(NATIVE)
        message(WARNING "-DNATIVE is not supported with MSVC")
    endif()
    # C4514: inline function not used
    # C4625: copy ctor implicitly deleted
    # C4626: copy assign implicitly deleted
    # C4820: padding added after member
    # C5026: move ctor implicitly deleted
    # C5027: move assign implicitly deleted
    target_compile_options(${target} PUBLIC
        $<$<BOOL:${STRICT}>:-Wall -WX -wd4820 -wd4514 -wd5026 -wd5027 -wd4626 -wd4625>
        )
endif()

install(TARGETS OSCMessenger LIBRARY DESTINATION ${PROJECT_NAME})
install(FILES src/OSCMessenger.sc DESTINATION ${PROJECT_NAME}/Classes)
install(FILES src/OSCMessenger.schelp DESTINATION ${PROJECT_NAME}/HelpSource/Classes)
