cd libgit2
mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=OFF -DBUILD_CLAR=OFF ..
cmake --build .
pause