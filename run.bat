mkdir build

cd build

cmake -G "MinGW Makefiles" ..
make -j4
copy %CD%\PicoWS2812B.uf2 E:\