﻿UpdateExternalLib("libjpeg-turbo" https://github.com/libjpeg-turbo/libjpeg-turbo.git "2.1.0")

if (WIN32)
  set(CMAKE_ASM_NASM_COMPILER "${PROJECT_SOURCE_DIR}/Tools/nasm-2.15.05-win64/nasm.exe")
elseif(APPLE)
  set(CMAKE_ASM_NASM_COMPILER "${PROJECT_SOURCE_DIR}/Tools/nasm-2.15.05-macosx/nasm")
endif()

set(LIBJPEG_TURBO_INSTALL_DIR ${CMAKE_BINARY_DIR}/libjpeg-turbo-install)
set(LIBJPEG_TURBO_SOURCE_DIR ${PROJECT_SOURCE_DIR}/ThirdParty/libjpeg-turbo)

# Set WITH_SIMD
include(CheckLanguage)
check_language(ASM_NASM)
if (CMAKE_ASM_NASM_COMPILER)
  option(WITH_SIMD "" ON)
else()
  option(WITH_SIMD "" OFF)
endif()
if (WITH_SIMD)
  message(STATUS "NASM assembler enabled")
else()
  message(STATUS "NASM assembler not found - libjpeg-turbo performance may suffer")
endif()

if(WIN32)
  execute_process(COMMAND ${CMAKE_COMMAND} 
    -G "${CMAKE_GENERATOR}" 
    -A "${CMAKE_GENERATOR_PLATFORM}" 
    -T "${CMAKE_GENERATOR_TOOLSET}" 
    -DCMAKE_CXX_FLAGS_RELEASE="/MD" 
    -DCMAKE_CXX_FLAGS_DEBUG="/MDd" 
    -DCMAKE_C_FLAGS_RELEASE="/MD" 
    -DCMAKE_C_FLAGS_DEBUG="/MDd" 
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -DWITH_CRT_DLL=ON 
    -DENABLE_STATIC=ON 
    -DENABLE_SHARED=OFF 
    -DWITH_JPEG7=ON 
    -DWITH_JPEG8=ON 
    -DWITH_TURBOJPEG=ON 
    -DCMAKE_ASM_NASM_COMPILER=${CMAKE_ASM_NASM_COMPILER} 
    -DREQUIRE_SIMD=${WITH_SIMD} 
    -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR} 
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
    -B ${CMAKE_BINARY_DIR}/ExternalProjects/libjpeg-turbo-external 
    WORKING_DIRECTORY ${LIBJPEG_TURBO_SOURCE_DIR}
  )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ExternalProjects/libjpeg-turbo-external
  )
else()
  execute_process(COMMAND ${CMAKE_COMMAND} 
    -G "${CMAKE_GENERATOR}" 
    -A "${CMAKE_GENERATOR_PLATFORM}" 
    -T "${CMAKE_GENERATOR_TOOLSET}" 
    -DCMAKE_MACOSX_RPATH=ON 
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_VERSION}" 
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -DWITH_CRT_DLL=ON 
    -DENABLE_STATIC=ON 
    -DENABLE_SHARED=OFF 
    -DWITH_JPEG7=ON 
    -DWITH_JPEG8=ON 
    -DWITH_TURBOJPEG=ON 
    -DCMAKE_ASM_NASM_COMPILER=${CMAKE_ASM_NASM_COMPILER} 
    -DREQUIRE_SIMD=${WITH_SIMD} 
    -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR} 
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
    -B ${CMAKE_BINARY_DIR}/ExternalProjects/libjpeg-turbo-external 
    WORKING_DIRECTORY ${LIBJPEG_TURBO_SOURCE_DIR}
  )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ExternalProjects/libjpeg-turbo-external
  )
endif()

set(LIBJPEG_TURBO_DIR "${CMAKE_BINARY_DIR}/libjpeg-turbo-install" CACHE PATH "libjpeg-turbo dir" FORCE)
set(LIBJPEG_TURBO_ROOT "${LIBJPEG_TURBO_DIR}" CACHE PATH "libjpeg-turbo root" FORCE)
