# DPS_A2
Some steps required to get it working.
First, install pistache (https://github.com/pistacheio/pistache): 
```
git clone https://github.com/oktal/pistache.git
cd pistache
git submodule update --init
mkdir -p {build,prefix}
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
```
And then, in the same `/pistache/build/` directory:
```
echo "export PKG_CONFIG_PATH=`pwd`:\$PKG_CONFIG_PATH" >> ~/.bashrc
source ~/.bashrc
```

The `client.cpp` file contains code for the client, the `worker.cpp` file contains code for the worker aka server. Compile them using `make` (this should just work after executing the steps above).


### DAS5 
It seems the above instructions do not hold up on the DAS5 cluster. Might have to do with the cmake version installed there, but further investigation is required.


### MILESTONES
- [x] connection between a 1-node server and a client (curl works)
- [ ] a client asks for connection at a master node, gets a worker node assigned, gets connection
- [ ] a workload is given to a client and result is returned
- [ ] workload distribution at master-node
- [ ] failure detection with heartbeats and or timeouts in server
- [ ] failure detection for client failure with heartbeats and or timeouts
- [ ] master node election system in case of failure
- [ ] workernode failure work recovery
- [ ] workernode failure client redistribution