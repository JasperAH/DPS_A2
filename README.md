# DiLae

A SETI@Home like system for workload distribution, supporting a varying amount of clients and worker-servers, currently implemented to calculate a dot product. 

## Setup
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
- [x] a client asks for connection at a master node, gets a worker node assigned, gets connection
- [x] a workload is given to a client and result is returned
- [x] workload distribution at master-node
- [x] failure detection with heartbeats and or timeouts in server
- [x] failure detection for client failure with heartbeats and or timeouts
- [x] master node election system in case of failure
- [x] workernode failure work recovery
- [x] workernode failure client redistribution (only when initial contact node is not down)



### Extra possible features
- [x] Dynamic redistribution of workload at master-node when workers are down or no more new worload is available
