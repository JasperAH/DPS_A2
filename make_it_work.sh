#!/bin/bash
cd pistache
git submodule update --init
cd build
cmake -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DPISTACHE_BUILD_EXAMPLES=false \
        -DPISTACHE_BUILD_TESTS=false \
        -DPISTACHE_BUILD_DOCS=false \
        -DPISTACHE_USE_SSL=false \
        -DCMAKE_INSTALL_PREFIX=$PWD/../prefix \
        ../
make -j
make install


echo "export PKG_CONFIG_PATH=/home/user/Documents/DPS/A2/DPS_A2/pistache/build/:\$PKG_CONFIG_PATH" >> ~/.bashrc
echo "use source ~/.bashrc to finish setup"