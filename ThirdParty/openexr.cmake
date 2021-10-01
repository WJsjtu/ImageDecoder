UpdateExternalLib("openexr" https://github.com/AcademySoftwareFoundation/openexr.git "RB-3.1")

set(OPENEXR_INSTALL_DIR "${CMAKE_BINARY_DIR}/openexr-install")
set(OPENEXR_SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/openexr")

file(READ "${OPENEXR_SOURCE_DIR}/src/test/OpenEXRCoreTest/compression.cpp" OPENEXR_COMPRESSION_TEST_CPP_CONTENT)
file(WRITE "${OPENEXR_SOURCE_DIR}/src/test/OpenEXRCoreTest/compression.cpp" "#include <iostream>\n#include <algorithm>\n${OPENEXR_COMPRESSION_TEST_CPP_CONTENT}")

if(WIN32)
  if(CMAKE_BUILD_TYPE MATCHES DEBUG OR CMAKE_BUILD_TYPE MATCHES RELWITHDEBINFO) 
    set(ZLIB_STATIC_LIBRARY ${ZLIB_DIR}/lib/zlibstaticd.lib CACHE FILEPATH "zlib library" FORCE)
    set(ZLIB_SHARED_DLL ${ZLIB_DIR}/bin/zlibd.dll CACHE FILEPATH "zlib library" FORCE)
    set(ZLIB_LIBRARY ${ZLIB_DIR}/lib/zlibd.lib CACHE FILEPATH "zlib library" FORCE)
  endif()
  if(CMAKE_BUILD_TYPE MATCHES RELEASE OR CMAKE_BUILD_TYPE MATCHES MINSIZEREL) 
    set(ZLIB_STATIC_LIBRARY ${ZLIB_DIR}/lib/zlibstatic.lib CACHE FILEPATH "zlib library" FORCE)
    set(ZLIB_SHARED_DLL ${ZLIB_DIR}/bin/zlib.dll CACHE FILEPATH "zlib library" FORCE)
    set(ZLIB_LIBRARY ${ZLIB_DIR}/lib/zlib.lib CACHE FILEPATH "zlib library" FORCE)
  endif()
else()
  if(CMAKE_BUILD_TYPE MATCHES DEBUG OR CMAKE_BUILD_TYPE MATCHES RELWITHDEBINFO) 
    set(ZLIB_STATIC_LIBRARY ${ZLIB_DIR}/lib/libz.a CACHE FILEPATH "zlib library" FORCE) 
    set(ZLIB_SHARED_DLL ${ZLIB_DIR}/lib/libz.dylib CACHE FILEPATH "zlib library" FORCE)
    set(ZLIB_LIBRARY ${ZLIB_DIR}/lib/libz.a CACHE FILEPATH "zlib library" FORCE)
  endif()
  if(CMAKE_BUILD_TYPE MATCHES RELEASE OR CMAKE_BUILD_TYPE MATCHES MINSIZEREL) 
    set(ZLIB_STATIC_LIBRARY ${ZLIB_DIR}/lib/libz.a CACHE FILEPATH "zlib library" FORCE) 
    set(ZLIB_SHARED_DLL ${ZLIB_DIR}/lib/libz.dylib CACHE FILEPATH "zlib library" FORCE)
    set(ZLIB_LIBRARY ${ZLIB_DIR}/lib/libz.a CACHE FILEPATH "zlib library" FORCE)
  endif()
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
    -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} /w4244 /w4264 /w4267 /w4819" 
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -DCMAKE_CXX_STANDARD=11 
    -DBUILD_SHARED_LIBS=OFF 
    -DOPENEXR_BUILD_TOOLS=ON 
    -DOPENEXR_RUN_FUZZ_TESTS=OFF 
    -DZLIB_INCLUDE_DIR=${ZLIB_DIR}/include 
    -DZLIB_LIBRARY=${ZLIB_LIBRARY} 
    -DCMAKE_INSTALL_PREFIX=${OPENEXR_INSTALL_DIR} 
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
    -B ${CMAKE_BINARY_DIR}/ExternalProjects/openexr-external 
    WORKING_DIRECTORY ${OPENEXR_SOURCE_DIR} 
  )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ExternalProjects/openexr-external
  )
else()
  execute_process(COMMAND ${CMAKE_COMMAND} 
    -G "${CMAKE_GENERATOR}" 
    -A "${CMAKE_GENERATOR_PLATFORM}" 
    -T "${CMAKE_GENERATOR_TOOLSET}" 
    -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} /w4244 /w4264 /w4267 /w4819" 
    -DCMAKE_MACOSX_RPATH=ON 
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_VERSION}" 
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -DCMAKE_CXX_STANDARD=11 
    -DBUILD_SHARED_LIBS=OFF 
    -DOPENEXR_BUILD_TOOLS=ON 
    -DOPENEXR_RUN_FUZZ_TESTS=OFF 
    -DZLIB_INCLUDE_DIR=${ZLIB_DIR}/include 
    -DZLIB_LIBRARY=${ZLIB_LIBRARY} 
    -DCMAKE_INSTALL_PREFIX=${OPENEXR_INSTALL_DIR} 
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
    -B ${CMAKE_BINARY_DIR}/ExternalProjects/openexr-external 
    WORKING_DIRECTORY ${OPENEXR_SOURCE_DIR} 
  )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ExternalProjects/openexr-external
  )
endif()

set(OPENEXR_DIR "${CMAKE_BINARY_DIR}/openexr-install" CACHE PATH "openexr dir" FORCE)
set(OPENEXR_ROOT "${OPENEXR_DIR}" CACHE PATH "openexr root" FORCE)