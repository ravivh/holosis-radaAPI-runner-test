Build with CMAKE
------------------

mkdir -p Build
rm -fr Build/*
cd Build/
cmake ../ -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-linux-gnu.cmake
make
