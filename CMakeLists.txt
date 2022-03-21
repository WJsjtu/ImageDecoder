cmake_minimum_required(VERSION 3.12)
set(PROJECT_NAME ImageDecoder)

set(COMPILER_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPILER_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

include(tools.cmake)

set(OSX_VERSION "10.9")

if (WIN32)
    project(${PROJECT_NAME} C CXX ASM)
    set (CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
if (APPLE)
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

option(BUILD_SHARED_LIBS "Build the shared library" ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_BUILD_TYPE MATCHES DEBUG OR CMAKE_BUILD_TYPE MATCHES RELWITHDEBINFO) 
elseif(CMAKE_BUILD_TYPE MATCHES RELEASE OR CMAKE_BUILD_TYPE MATCHES MINSIZEREL)
else()
set(CMAKE_BUILD_TYPE "MINSIZEREL")
endif()

file (
    GLOB_RECURSE image_src
    LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/Source/*.cpp"
    "${PROJECT_SOURCE_DIR}/Source/*.h"
    )

set(LIBRARY_NAME "image")

if(BUILD_SHARED_LIBS)
  add_library(${LIBRARY_NAME} SHARED ${image_src})
else()
  add_library(${LIBRARY_NAME} STATIC ${image_src})
  target_compile_definitions(${LIBRARY_NAME} PUBLIC IMAGE_STATIC)
endif()

add_subdirectory(ThirdParty)

if(WIN32)
  target_link_libraries(${LIBRARY_NAME} ${LIBJPEG_TURBO_DIR}/$<IF:$<CONFIG:Debug>,lib/jpeg-static.lib,lib/jpeg-static.lib>)
  target_link_libraries(${LIBRARY_NAME} ${LIBJPEG_TURBO_DIR}/$<IF:$<CONFIG:Debug>,lib/turbojpeg-static.lib,lib/turbojpeg-static.lib>)
else()
  target_link_libraries(${LIBRARY_NAME} ${LIBJPEG_TURBO_DIR}/$<IF:$<CONFIG:Debug>,lib/libjpeg.a,lib/libjpeg.a>)
  target_link_libraries(${LIBRARY_NAME} ${LIBJPEG_TURBO_DIR}/$<IF:$<CONFIG:Debug>,lib/libturbojpeg.a,lib/libturbojpeg.a>)
endif()

if(WIN32)
  target_link_libraries(${LIBRARY_NAME} ${LIBPNG_DIR}/$<IF:$<CONFIG:Debug>,lib/libpng16_staticd.lib,lib/libpng16_static.lib>)
else()
  target_link_libraries(${LIBRARY_NAME} ${LIBPNG_DIR}/$<IF:$<CONFIG:Debug>,lib/libpng16d.a,lib/libpng16.a>)
endif()

if(WIN32)
  target_link_libraries(${LIBRARY_NAME} ${ZLIB_DIR}/$<IF:$<CONFIG:Debug>,lib/zlibstaticd.lib,lib/zlibstatic.lib>)
else()
  target_link_libraries(${LIBRARY_NAME} ${ZLIB_DIR}/$<IF:$<CONFIG:Debug>,lib/libz.a,lib/libz.a>)
endif()

if(WIN32)
  file (
    GLOB_RECURSE OPENEXR_LIBRARY_FILE
    LIST_DIRECTORIES false
    "${OPENEXR_DIR}/lib/*.lib"
  )
  foreach(x IN LISTS OPENEXR_LIBRARY_FILE)
    target_link_libraries(${LIBRARY_NAME} ${x})
  endforeach()
else()
  file (
    GLOB_RECURSE OPENEXR_LIBRARY_FILE
    LIST_DIRECTORIES false
    "${OPENEXR_DIR}/lib/*.a"
  )
  foreach(x IN LISTS OPENEXR_LIBRARY_FILE)
    target_link_libraries(${LIBRARY_NAME} ${x})
  endforeach()
endif()

target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/Source")
target_include_directories(${LIBRARY_NAME} PRIVATE "${LIBPNG_DIR}/include")
target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/libpng")
target_include_directories(${LIBRARY_NAME} PRIVATE "${LIBJPEG_TURBO_DIR}/include")
target_include_directories(${LIBRARY_NAME} PRIVATE "${ZLIB_DIR}/include")
target_include_directories(${LIBRARY_NAME} PRIVATE "${OPENEXR_DIR}/include" "${OPENEXR_DIR}/include/Imath" "${OPENEXR_DIR}/include/OpenEXR")
target_compile_definitions(${LIBRARY_NAME} PRIVATE IMAGE_DLL)

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

if(BUILD_SHARED_LIBS)
  if(WIN32)
    install(FILES $<TARGET_PDB_FILE:${LIBRARY_NAME}> DESTINATION bin OPTIONAL)
  else()
    target_compile_options(${LIBRARY_NAME} PUBLIC -fvisibility=hidden)
    target_compile_options(${LIBRARY_NAME} PUBLIC -fvisibility-inlines-hidden)
  endif()
else()
  get_target_property(linked_libs ${LIBRARY_NAME} LINK_LIBRARIES)
  install(FILES ${linked_libs} DESTINATION lib)
endif()

# ============ Test ==============
file (
  GLOB_RECURSE main_src
  LIST_DIRECTORIES false
  "${PROJECT_SOURCE_DIR}/Test/*.cpp"
  "${PROJECT_SOURCE_DIR}/Test/*.h"
)

add_executable(${PROJECT_NAME} ${main_src})
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/Source")

install_glfw(${PROJECT_NAME})

if(BUILD_SHARED_LIBS)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${LIBRARY_NAME}>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
  )
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARY_NAME})

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}
RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
LIBRARY DESTINATION "${INSTALL_LIB_DIR}")

if (APPLE AND BUILD_SHARED_LIBS)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")
  set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_BUILD_RPATH "$ORIGIN")
  set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_INSTALL_RPATH "$ORIGIN")
  install(CODE "file(COPY ${CMAKE_INSTALL_PREFIX}/lib/libimage.dylib DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_targets_folder()