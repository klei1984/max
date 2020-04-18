export PATH=$PATH:/mingw32/bin
mkdir $BUILD_DIR
cd $BUILD_DIR

cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX="C:/Program Files (x86)/max" -DSDL2_DIR=/mingw32/lib/cmake/SDL2 ..
cmake --build .

cpack
mkdir Artifacts
cp *-win32.exe Artifacts/
