# DPS_A2
Some steps required to get it working.
Starting from DPS_A2 directory:
```
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


echo "export PKG_CONFIG_PATH=`pwd`:\$PKG_CONFIG_PATH" >> ~/.bashrc
source ~/.bashrc
```

The `client.cpp` file contains code for the client, the `worker.cpp` file contains code for the worker aka server. Compile them using `make` (this should just work after executing the steps above).
