cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
include(ExternalProject)

set(PROJECT_NAME image)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(COMPILER_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPILER_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(OSX_VERSION "10.9")

if(WIN32)
  project(${PROJECT_NAME} C CXX ASM)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "${OSX_VERSION}" CACHE STRING "Minimum OS X deployment version")
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "OS X archs")
  project(${PROJECT_NAME} C CXX OBJC ASM)
  set(CMAKE_C_FLAGS "-fobjc-arc")
  set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++ -fobjc-arc")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
  set(CMAKE_MACOSX_RPATH ON)
  set(BUILD_RPATH_USE_ORIGIN TRUE)
  set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
endif()

# zlib
set(ZLIB_INSTALL_DIR "${PROJECT_BINARY_DIR}/ThirdParty/zlib")

if(WIN32)
  find_package(Git QUIET)

  if(NOT GIT_FOUND)
    message(FATAL_ERROR "Git is not available.")
  endif()

  execute_process(COMMAND "${GIT_EXECUTABLE} reset --hard" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ThirdParty/zlib"
    RESULT_VARIABLE GIT_RESET_RESULT)
  execute_process(COMMAND "${GIT_EXECUTABLE} apply ${PROJECT_SOURCE_DIR}/zlib.patch" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ThirdParty/zlib"
    RESULT_VARIABLE GIT_PATCH_RESULT)
  file(REMOVE_RECURSE "${PROJECT_BINARY_DIR}/ThirdParty/zlib/src")
  file(COPY "${PROJECT_SOURCE_DIR}/ThirdParty/zlib" DESTINATION "${PROJECT_BINARY_DIR}/ThirdParty/zlib/src")
  execute_process(COMMAND "${GIT_EXECUTABLE} reset --hard" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ThirdParty/zlib"
    RESULT_VARIABLE GIT_RESET_RESULT)
  ExternalProject_Add(zlib
    SOURCE_DIR "${PROJECT_BINARY_DIR}/ThirdParty/zlib/src/zlib"
    CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_INSTALL_PREFIX=${ZLIB_INSTALL_DIR}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  )
elseif(APPLE)
  ExternalProject_Add(zlib
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/zlib"
    CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_INSTALL_PREFIX=${ZLIB_INSTALL_DIR}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_MACOSX_RPATH=ON
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_VERSION}"
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
  )
endif()

# libpng
set(LIBPNG_INSTALL_DIR "${PROJECT_BINARY_DIR}/ThirdParty/libpng")

if(WIN32)
  ExternalProject_Add(libpng
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/libpng"
    CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DPNG_SHARED=OFF
    -DPNG_EXECUTABLES=OFF
    -DZLIB_INCLUDE_DIR=${ZLIB_INSTALL_DIR}/include
    -DZLIB_LIBRARY=${ZLIB_INSTALL_DIR}/lib/$<IF:$<CONFIG:Debug>,zlibstaticd.lib,zlibstatic.lib>
    -DCMAKE_INSTALL_PREFIX=${LIBPNG_INSTALL_DIR}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_CXX_FLAGS_DEBUG="/MTd"
    -DCMAKE_CXX_FLAGS_RELEASE="/MT"
    -DCMAKE_C_FLAGS_DEBUG="/MTd"
    -DCMAKE_C_FLAGS_RELEASE="/MT"
    DEPENDS zlib
  )
elseif(APPLE)
  if("${CMAKE_OSX_ARCHITECTURES}" MATCHES "^arm")
    set(PNG_ARM_NEON off)
  else()
    set(PNG_ARM_NEON on)
  endif()

  ExternalProject_Add(libpng
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/libpng"
    CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DPNG_SHARED=OFF
    -DPNG_EXECUTABLES=OFF
    -DZLIB_INCLUDE_DIR=${ZLIB_INSTALL_DIR}/include
    -DZLIB_LIBRARY=${ZLIB_INSTALL_DIR}/lib/libz.a
    -DCMAKE_INSTALL_PREFIX=${LIBPNG_INSTALL_DIR}
    -DPNG_ARM_NEON=${PNG_ARM_NEON}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_MACOSX_RPATH=ON
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_VERSION}"
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    DEPENDS zlib
  )
endif()

ExternalProject_Add_Step(libpng fixup-install
  COMMAND ${CMAKE_COMMAND} -E copy
  ${PROJECT_SOURCE_DIR}/ThirdParty/libpng/pngstruct.h
  ${LIBPNG_INSTALL_DIR}/include/pngstruct.h
)

ExternalProject_Add_Step(libpng fixup-install2
  COMMAND ${CMAKE_COMMAND} -E copy
  ${PROJECT_SOURCE_DIR}/ThirdParty/libpng/pnginfo.h
  ${LIBPNG_INSTALL_DIR}/include/pnginfo.h
)

# libjpeg-turbo
set(LIBJPEG_TURBO_INSTALL_DIR "${PROJECT_BINARY_DIR}/ThirdParty/libjpeg-turbo")

if(WIN32)
  set(CMAKE_ASM_NASM_COMPILER "${PROJECT_SOURCE_DIR}/Tools/nasm-2.15.05-win64/nasm.exe")
elseif(APPLE)
  execute_process(COMMAND chmod -R 777 "${PROJECT_SOURCE_DIR}/Tools")
  set(CMAKE_ASM_NASM_COMPILER "${PROJECT_SOURCE_DIR}/Tools/nasm-2.15.05-macosx/nasm")
endif()

include(CheckLanguage)
check_language(ASM_NASM)

if(CMAKE_ASM_NASM_COMPILER)
  option(WITH_SIMD "" ON)
else()
  option(WITH_SIMD "" OFF)
endif()

if(WIN32)
  ExternalProject_Add(libjpeg-turbo
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/libjpeg-turbo"
    CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
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
    -DCMAKE_CXX_FLAGS_DEBUG="/MTd"
    -DCMAKE_CXX_FLAGS_RELEASE="/MT"
    -DCMAKE_C_FLAGS_DEBUG="/MTd"
    -DCMAKE_C_FLAGS_RELEASE="/MT"
  )
elseif(APPLE)
  ExternalProject_Add(libjpeg-turbo
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/libjpeg-turbo"
    CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
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
    -DCMAKE_MACOSX_RPATH=ON
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_VERSION}"
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
  )
endif()

# openexr
set(OPENEXR_INSTALL_DIR "${PROJECT_BINARY_DIR}/ThirdParty/openexr")

if(WIN32)
  ExternalProject_Add(openexr
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/openexr"
    CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} /w4244 /w4264 /w4267 /w4819"
    -DCMAKE_CXX_STANDARD=11
    -DBUILD_SHARED_LIBS=OFF
    -DOPENEXR_BUILD_TOOLS=OFF
    -DOPENEXR_RUN_FUZZ_TESTS=OFF
    -DBUILD_TESTING=OFF
    -DBUILD_WEBSITE=OFF
    -DOPENEXR_BUILD_PYTHON=OFF
    -DOPENEXR_INSTALL_DOCS=OFF
    -DOPENEXR_INSTALL_EXAMPLES=OFF
    -DOPENEXR_FORCE_INTERNAL_IMATH=ON
    -DOPENEXR_LIB_SUFFIX=
    -DOPENEXR_IMATH_TAG=v3.1.9
    -DZLIB_INCLUDE_DIR=${ZLIB_INSTALL_DIR}/include
    -DZLIB_LIBRARY=${ZLIB_INSTALL_DIR}/lib/$<IF:$<CONFIG:Debug>,zlibstaticd.lib,zlibstatic.lib>
    -DCMAKE_INSTALL_PREFIX=${OPENEXR_INSTALL_DIR}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_CXX_FLAGS_DEBUG="/MTd"
    -DCMAKE_CXX_FLAGS_RELEASE="/MT"
    -DCMAKE_C_FLAGS_DEBUG="/MTd"
    -DCMAKE_C_FLAGS_RELEASE="/MT"
    DEPENDS zlib
  )
elseif(APPLE)
  ExternalProject_Add(openexr
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/openexr"
    CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_STANDARD=11
    -DBUILD_SHARED_LIBS=OFF
    -DOPENEXR_BUILD_TOOLS=OFF
    -DOPENEXR_RUN_FUZZ_TESTS=OFF
    -DBUILD_TESTING=OFF
    -DBUILD_WEBSITE=OFF
    -DOPENEXR_BUILD_PYTHON=OFF
    -DOPENEXR_INSTALL_DOCS=OFF
    -DOPENEXR_INSTALL_EXAMPLES=OFF
    -DOPENEXR_FORCE_INTERNAL_IMATH=ON
    -DOPENEXR_LIB_SUFFIX=
    -DOPENEXR_IMATH_TAG=v3.1.9
    -DZLIB_INCLUDE_DIR=${ZLIB_INSTALL_DIR}/include
    -DZLIB_LIBRARY=${ZLIB_INSTALL_DIR}/lib/libz.a
    -DCMAKE_INSTALL_PREFIX=${OPENEXR_INSTALL_DIR}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_MACOSX_RPATH=ON
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_VERSION}"
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    DEPENDS zlib
  )
endif()

# library
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(
  GLOB_RECURSE image_src
  LIST_DIRECTORIES false
  "${PROJECT_SOURCE_DIR}/Source/*.cpp"
  "${PROJECT_SOURCE_DIR}/Source/*.h"
)

set(LIBRARY_NAME "image")
add_library(${LIBRARY_NAME} SHARED ${image_src})
set_property(TARGET ${LIBRARY_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(${LIBRARY_NAME} PRIVATE IMAGE_DLL)
target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/Source")
target_include_directories(${LIBRARY_NAME} PRIVATE "${ZLIB_INSTALL_DIR}/include")
target_include_directories(${LIBRARY_NAME} PRIVATE "${LIBPNG_INSTALL_DIR}/include")
target_include_directories(${LIBRARY_NAME} PRIVATE "${LIBJPEG_TURBO_INSTALL_DIR}/include")
target_include_directories(${LIBRARY_NAME} PRIVATE "${OPENEXR_INSTALL_DIR}/include" "${OPENEXR_INSTALL_DIR}/include/Imath" "${OPENEXR_INSTALL_DIR}/include/OpenEXR")

if(WIN32)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBJPEG_TURBO_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/jpeg-static.lib,lib/jpeg-static.lib>)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBJPEG_TURBO_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/turbojpeg-static.lib,lib/turbojpeg-static.lib>)
elseif(APPLE)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBJPEG_TURBO_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/libjpeg.a,lib/libjpeg.a>)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBJPEG_TURBO_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/libturbojpeg.a,lib/libturbojpeg.a>)
endif()

if(WIN32)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBPNG_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/libpng16_staticd.lib,lib/libpng16_static.lib>)
elseif(APPLE)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBPNG_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/libpng16d.a,lib/libpng16.a>)
endif()

if(WIN32)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${ZLIB_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/zlibstaticd.lib,lib/zlibstatic.lib>)
elseif(APPLE)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${ZLIB_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/libz.a,lib/libz.a>)
endif()

if(WIN32)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${OPENEXR_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/Iex_d.lib,lib/Iex.lib>)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${OPENEXR_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/IlmThread_d.lib,lib/IlmThread.lib>)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${OPENEXR_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/OpenEXR_d.lib,lib/OpenEXR.lib>)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${OPENEXR_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/OpenEXRCore_d.lib,lib/OpenEXRCore.lib>)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${OPENEXR_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/OpenEXRUtil_d.lib,lib/OpenEXRUtil.lib>)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${OPENEXR_INSTALL_DIR}/$<IF:$<CONFIG:Debug>,lib/Imath-3_1_d.lib,lib/Imath-3_1.lib>)
elseif(APPLE)
endif()

add_dependencies(${LIBRARY_NAME} libpng libjpeg-turbo openexr)

if(WIN32)
elseif(APPLE)
endif()

set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Installation directory for executables")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "Installation directory for libraries")
set(INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "Installation directory for headers")

install(TARGETS ${LIBRARY_NAME} ${LIBRARY_NAME}
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}")

set(LIBRARY_PUBLIC_HDRS
  ${PROJECT_SOURCE_DIR}/Source/Decoder.h
)

install(FILES ${LIBRARY_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")

if(WIN32)
  install(FILES $<TARGET_PDB_FILE:${LIBRARY_NAME}> DESTINATION bin OPTIONAL)
elseif(APPLE)
  target_compile_options(${LIBRARY_NAME} PUBLIC -fvisibility=hidden)
  target_compile_options(${LIBRARY_NAME} PUBLIC -fvisibility-inlines-hidden)
endif()

# ============ Test ==============
file(
  GLOB_RECURSE main_src
  LIST_DIRECTORIES false
  "${PROJECT_SOURCE_DIR}/Test/*.cpp"
  "${PROJECT_SOURCE_DIR}/Test/*.h"
)

set(main_name ${PROJECT_NAME}_viewer)
add_executable(${main_name} ${main_src})
target_include_directories(${main_name} PUBLIC "${PROJECT_SOURCE_DIR}/Source")

set(GLFW_LIBRARY_TYPE "STATIC")

# set(USE_MSVC_RUNTIME_LIBRARY_DLL off)
add_subdirectory("${PROJECT_SOURCE_DIR}/ThirdParty/glfw")
set(glew-cmake_BUILD_SHARED off)
add_subdirectory("${PROJECT_SOURCE_DIR}/ThirdParty/glew-cmake")

# set_property(TARGET libglew_static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_include_directories(${main_name} PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/glfw/include")
target_include_directories(${main_name} PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/glew-cmake/include")
target_link_libraries(${main_name} PRIVATE ${LIBRARY_NAME} PRIVATE glfw PRIVATE libglew_static)

if(WIN32)
  target_compile_definitions(glfw PRIVATE GLFW_EXPOSE_NATIVE_WIN32)
  target_compile_definitions(${main_name} PRIVATE GLFW_EXPOSE_NATIVE_WIN32)
  target_link_libraries(${main_name} PRIVATE opengl32.lib)
elseif(APPLE)
  target_compile_definitions(glfw PRIVATE GLFW_EXPOSE_NATIVE_COCOA)
  target_compile_definitions(${main_name} PRIVATE GLFW_EXPOSE_NATIVE_COCOA)
  find_library(OPENGL OpenGL)
  target_link_libraries(${main_name} PRIVATE ${OPENGL})
  find_library(APPKIT AppKit)
  target_link_libraries(${main_name} PRIVATE ${APPKIT})
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(${main_name} PRIVATE ${CORE_FOUNDATION})
  find_library(CORE_GRAPHICS CoreGraphics)
  target_link_libraries(${main_name} PRIVATE ${CORE_GRAPHICS})
  find_library(QUARTZ_CORE QuartzCore)
  target_link_libraries(${main_name} PRIVATE ${QUARTZ_CORE})
endif()

add_custom_command(TARGET ${main_name} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${LIBRARY_NAME}>" "$<TARGET_FILE_DIR:${main_name}>"
)

# target_link_libraries(${main_name} PRIVATE ${LIBRARY_NAME})

# set_property(TARGET ${main_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
install(TARGETS ${main_name} ${main_name}
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}")

if(APPLE AND BUILD_SHARED_LIBS)
  set_target_properties(${main_name} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")
  set_target_properties(${main_name} PROPERTIES CMAKE_BUILD_RPATH "$ORIGIN")
  set_target_properties(${main_name} PROPERTIES CMAKE_INSTALL_RPATH "$ORIGIN")
  install(CODE "file(COPY ${CMAKE_INSTALL_PREFIX}/lib/libimage.dylib DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)")
endif()