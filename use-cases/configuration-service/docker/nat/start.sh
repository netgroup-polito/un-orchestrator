#!/bin/bash

hs="$(cat /etc/hostname)" && echo "127.0.1.1 $hs" >> /etc/hosts
./start_nat.sh
sleep 1
./start_nat_agent.sh
