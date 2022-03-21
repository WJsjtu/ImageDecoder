cmake -B ./Build/Debug -G "Visual Studio 15 2017" -A x64 -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX=./Install/Debug
cmake --build ./Build/Debug --config DEBUG --target install

cmake -B ./Build/MinSizeRel -G "Visual Studio 15 2017" -A x64 -DCMAKE_BUILD_TYPE=MINSIZEREL -DCMAKE_INSTALL_PREFIX=./Install/MinSizeRel
cmake --build ./Build/MinSizeRel --config MINSIZEREL --target install