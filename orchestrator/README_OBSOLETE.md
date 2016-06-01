# Obsolete documentation

This file contains some documentation related to features that were supported in the past and that are now no longer maintained and supported.

### Installing Open vSwitch with OFCONFIG [DEPRECATED]

Open vSwitch can be installed with either the OVSDB or OF-CONFIG plugin.
Although both protocols allow to control the switch (e.g., create/delete
new bridging instances, create/delete ports, etc), we found out
that OF-CONFIG is rather limited in terms of capabilities. For instance,
it cannot set the type of port configured on the switch (e.g., virtio
or IVSHMEM), requiring the orchestrator to rely on a combination of
OF-CONFIG commands and bash scripts to perform its job.

For this reason we suggest to install OpenvSwitch with its native OSVDB
support (next section); although OVSDB is not standard, it seems that it
does its job better than OF-CONFIG.

In any case, the compilation instruction for setting up OpenvSwitch with
OF-CONFIG are the following (not guaranteed that those are 100% accurate,
as the OF-CONFIG support in OpenvSwitch is rather primitive).

OvS with the OFCONFIG support can be installed as follows:

	$ sudo apt-get install autoconf automake gcc libtool libxml2 libxml2-dev m4 make openssl dbus

	; Download LIBSSH from
	;       https://red.libssh.org/projects/libssh/files
	; Now install the above library following the INSTALL file provided in the root directory

	; Clone the libnetconf repository
	$ git clone https://github.com/cesnet/libnetconf
    $ cd libnetconf/
    $ git checkout -b 0.9.x origin/0.9.x

	; Install the libnetconf library by following the instructions in the
    ; INSTALL file contained in the root folder of this library.

    ; Download OpenvSwitch from
    ;      from http://openvswitch.org/releases/openvswitch-2.4.0.tar.gz
    $ tar -xf openvswitch-2.4.0.tar.gz
    $ cd openvswitch-2.4.0
    $ ./configure --prefix=/ --datarootdir=/usr/share --with-linux=/lib/modules/$(uname -r)/build
    $ make
    $ sudo make install

	; Clone the of-config repository
	$ git clone https://github.com/openvswitch/of-config

	; Follow the instructions as described in the file INSTALL.md provided in the root folder of that repository.


### How to start OvS (managed through OFCONFIG) to work with the un-orchestrator [DEPRECATED]

Start OvS:

    $ sudo /usr/share/openvswitch/scripts/ovs-ctl start

In addition, you have to start the OF-CONFIG server, which represents the
daemon the implements the protocol used to configure the switch.

OF-CONFIG server can be started by:

    $ sudo ofc-server

By default, `ofc-server` starts in daemon mode. To avoid daemon mode, use the
`-f` parameter.
For the full list of the supported parameters, type:

    $ ofc-server -h
