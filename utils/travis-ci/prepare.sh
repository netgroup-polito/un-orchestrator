#!/bin/bash
set -ev	#print every line before executing it and exit if one command fails

BASE=`pwd`

# rofl
cd $HOME
ls
if [ ! -d "rofl-common/build" ]; then
	git clone https://github.com/bisdn/rofl-common
	cd rofl-common/
	git checkout stable-0.6
	./autogen.sh
	cd build
	../configure
	make -j
else
	echo "rofl-common exists"
	cd rofl-common/build
fi

sudo make install

# json-spirit
cd $HOME
if [ ! -d "json-spirit/build" ]; then
	git clone https://github.com/sirikata/json-spirit
	cd json-spirit/build
	cmake .
	make -j
else
	echo "json-spirit exists"
	cd json-spirit/build
fi

sudo make install

# double decker prerequisites
if [ "$DD" != "ON" ]; then
	exit 0
fi

cd $HOME
if [ ! -d "double_decker/DoubleDecker" ]; then
	echo "****Double decker cache not found****"
	cd double_decker

	# - zeromq
	cd $HOME/double_decker
	wget http://github.com/zeromq/czmq/archive/v3.0.2.tar.gz
	tar xvfz v3.0.2.tar.gz
	cd czmq-3.0.2
	./autogen.sh
	./configure --prefix=/usr
	make -j
	sudo make install

	# - urcu
	cd $HOME/double_decker
	wget http://www.lttng.org/files/urcu/userspace-rcu-0.9.1.tar.bz2
	tar xvfj userspace-rcu-0.9.1.tar.bz2
	cd userspace-rcu-0.9.1
	./configure --prefix=/usr
	make -j
	sudo make install

	# - libsodium
	cd $HOME/double_decker
	wget http://download.libsodium.org/libsodium/releases/libsodium-1.0.7.tar.gz
	tar xvfz libsodium-1.0.7.tar.gz
	cd libsodium-1.0.7
	./configure
	make -j
	sudo make install

	# double decker
	cd $HOME/double_decker
	git clone https://github.com/Acreo/DoubleDecker
	cd DoubleDecker
	./boot.sh
	./configure
	make -j
	sudo make install
else
	echo "****Double decker cache found****"
	#everything should be compiled, install it
	cd $HOME/double_decker/czmq-3.0.2
	sudo make install

	cd $HOME/double_decker/userspace-rcu-0.9.1
	sudo make install

	cd $HOME/double_decker/libsodium-1.0.7
	sudo make install

	cd $HOME/double_decker/DoubleDecker
	sudo make install
fi


