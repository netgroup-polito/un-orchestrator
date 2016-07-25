#!/bin/bash

set -ev # print every line before executing it and exit if one command fails

# build name resolver and orchestrator
cmake -DCMAKE_BUILD_TYPE=Release \
 -DENABLE_KVM=$KVM -DENABLE_DOCKER=$DOCKER \
 -DENABLE_NATIVE=$NATIVE \
 -DENABLE_DPDK_PROCESSES=$DPDK \
 -DVSWITCH_IMPLEMENTATION=$VSWITCH \
 -DENABLE_DOUBLE_DECKER_CONNECTION=$DD \
 -DENABLE_RESOURCE_MANAGER=$DD \
 -DBUILD_ExternalProjects=OFF .

make -j2
