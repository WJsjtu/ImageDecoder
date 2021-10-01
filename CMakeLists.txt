cmake_minimum_required(VERSION 3.18)
set(PROJECT_NAME ImageDecoder)

set(COMPILER_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPILER_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(OSX_VERSION "10.9")

if (WIN32)
    project(${PROJECT_NAME} C CXX ASM)
    set (CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${OSX_VERSION}" CACHE STRING "Minimum OS X deployment version")
    project(${PROJECT_NAME} C CXX OBJC ASM)
    set(CMAKE_C_FLAGS "-fobjc-arc")
    set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++ -fobjc-arc")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
    set(CMAKE_MACOSX_RPATH ON)
    set(BUILD_RPATH_USE_ORIGIN TRUE)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_BUILD_TYPE MATCHES DEBUG OR CMAKE_BUILD_TYPE MATCHES RELWITHDEBINFO) 
elseif(CMAKE_BUILD_TYPE MATCHES RELEASE OR CMAKE_BUILD_TYPE MATCHES MINSIZEREL)
else()
set(CMAKE_BUILD_TYPE "MINSIZEREL")
endif()

file (
    GLOB_RECURSE image_src
    LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/SOurce/*.cpp"
    "${PROJECT_SOURCE_DIR}/SOurce/*.h"
    )

foreach(_source IN ITEMS ${image_src})
    if (IS_ABSOLUTE "${_source}")
        file(RELATIVE_PATH _source_rel "${PROJECT_SOURCE_DIR}" "${_source}")
    else()
        set(_source_rel "${_source}")
    endif()
    get_filename_component(_source_path "${_source_rel}" PATH)
    if (WIN32)
        string(REPLACE "/" "\\" _source_path_replaced "${_source_path}")
    else()
        set(_source_path_replaced "${_source_path}")
    endif()
    source_group("${_source_path_replaced}" FILES "${_source}")
endforeach()

set(LIBRARY_NAME "image")

add_library(${LIBRARY_NAME} SHARED ${image_src})

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
target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/Source/wrapper")
target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/Source/Decoder")
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
  ${PROJECT_SOURCE_DIR}/Source/Export.h 
  ${PROJECT_SOURCE_DIR}/Source/Wrapper/IImageWrapper.h
  ${PROJECT_SOURCE_DIR}/Source/Wrapper/IImageWrapperModule.h
  ${PROJECT_SOURCE_DIR}/Source/Decoder/Decoder.h
)

install(FILES ${LIBRARY_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")
install(FILES $<TARGET_PDB_FILE:${LIBRARY_NAME}> DESTINATION bin OPTIONAL)