UpdateExternalLib("libpng" https://github.com/glennrp/libpng.git "v1.6.37")
set(LIBPNG_INSTALL_DIR ${CMAKE_BINARY_DIR}/libpng-install)
set(LIBPNG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/ThirdParty/libpng)

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
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -DPNG_SHARED=OFF 
    -DPNG_EXECUTABLES=OFF 
    -DZLIB_INCLUDE_DIR=${ZLIB_DIR}/include 
    -DZLIB_LIBRARY=${ZLIB_LIBRARY} 
    -DCMAKE_INSTALL_PREFIX=${LIBPNG_INSTALL_DIR} 
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
    -B ${CMAKE_BINARY_DIR}/ExternalProjects/libpng-external
    WORKING_DIRECTORY ${LIBPNG_SOURCE_DIR}
  )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ExternalProjects/libpng-external
  )
else()
  execute_process(COMMAND ${CMAKE_COMMAND} 
    -G "${CMAKE_GENERATOR}" 
    -A "${CMAKE_GENERATOR_PLATFORM}" 
    -T "${CMAKE_GENERATOR_TOOLSET}" 
    -DCMAKE_MACOSX_RPATH=ON 
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_VERSION}" 
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -DPNG_SHARED=OFF 
    -DPNG_EXECUTABLES=OFF 
    -DZLIB_INCLUDE_DIR=${ZLIB_DIR}/include 
    -DZLIB_LIBRARY=${ZLIB_LIBRARY} 
    -DCMAKE_INSTALL_PREFIX=${LIBPNG_INSTALL_DIR} 
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
    -B ${CMAKE_BINARY_DIR}/ExternalProjects/libpng-external
    WORKING_DIRECTORY ${LIBPNG_SOURCE_DIR}
  )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ExternalProjects/libpng-external
  )
endif()

set(LIBPNG_DIR "${CMAKE_BINARY_DIR}/libpng-install" CACHE PATH "libpng dir" FORCE)
set(LIBPNG_ROOT "${LIBPNG_DIR}" CACHE PATH "libpng root" FORCE)