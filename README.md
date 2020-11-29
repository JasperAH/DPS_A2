# DPS_A2
Some steps required to get it working.
First, install pistache (https://github.com/pistacheio/pistache): 
```
git clone https://github.com/oktal/pistache.git
git submodule update --init
cd pistache
mkdir -p {build,prefix}
cd build
cmake -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DPISTACHE_BUILD_EXAMPLES=true \
        -DPISTACHE_BUILD_TESTS=true \
        -DPISTACHE_BUILD_DOCS=false \
        -DPISTACHE_USE_SSL=false \
        -DCMAKE_INSTALL_PREFIX=$PWD/../prefix \
        ../
make -j
make install
```
And then, in the same `/pistache/build/` directory:
```
echo "export PKG_CONFIG_PATH=`pwd`:\$PKG_CONFIG_PATH" >> ~/.bashrc
source ~/.bashrc
```

The `client.cpp` file contains code for the client, the `worker.cpp` file contains code for the worker aka server. Compile them using `make` (this should just work after executing the steps above).
