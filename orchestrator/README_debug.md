# Debugging a UN service

This document provides some suggestions for debugging the UN when the
deployed service (that comes as NF-FG) does not work as expected.

In this case, the problem may be that the UN translates the NF-FG with
the wrong set of directives, and the following commands help you to
understand what is going on.


## Debug OvS

The following commands are useful to check the status of OvS.

    ; Show the bridges created with the respective ports
    $ sudo ovs-vsctl show

    ; Show the OpenFlow identifiers for the ports of a bridge
    $ sudo ovs-ofctl show <bridge_name> --protocol=Openflow12

    ; Show the forwarding table of a bridge
    $ sudo ovs-ofctl dump-flows <bridge_name> --protocol=Openflow12

    ; Delete a bridge
    $ sudo ovs-vsctl del-br <bridge_name>

Please remember that the UN usually creates serveral Logical
Switching Instances (LSI), which are shown by OVS as different
bridges. Hence you may need to debug several bridges to see what
is going on.


## Debug OpenFlow messages

The following commands show how to setup Wireshark to debug OpenFlow messages

	; Install Wireshark
	$ sudo apt-get install wireshark

	; Move in the Wireshark plugin directory
	$ cd /usr/lib/x86_64-linux-gnu/wireshark/libwireshark3/plugins

	; Download the OpenFlow dissector plugin
	$ wget http://www.projectfloodlight.org/openflow.lua

	; For each controller port different from well known TCP ports for
	OpenFlow (6633 and 6653) exec the following command:
	$ echo "tcp_dissector_table:add(#PORT, p_of)" >> \
	  /usr/lib/x86_64-linux-gnu/wireshark/libwireshark3/plugins/openflow.lua
	; where #PORT is the port number of the controller

Now Wireshark recognizes Openflow messages.
Run it and capture traffic on the loopback interface (the UN orchestrator and the vSwitch are executed on the same machine).


## Debug Docker

The following commands are useful to check the status of the Docker environment

    ; Show the running containers
    $ sudo docker ps

    ; Show the containers that are no longer running but that have not been removed
    $ sudo docker ps -a

    ; Execute a command in a running container
    $ sudo docker <container_id> <command>
    ; The container ID can be retrieved through the first command above

    ; Kill a container (it will not be shown anymore with `docker ps`)
    $ sudo docker kill <container_id>

    ; Remove a container
    $ sudo docker rm <container_id>

	; Read the log of a VNF
	$ cd [un-orchestrator]/orchestrator
	$ cat <container_name>.log
	; The container name can be retrieved through `sudo docker ps` 


## Debug KVM

The UN orchestrator manages the virtual machines using libvirt, it means that you can access all the information related to the running instances using the `virsh` tool.
The version of `virsh` to be used in the one in `/usr/local/bin`.

The following are examples of the most important commands, for the full list please visit the [Virsh Command Reference](http://libvirt.org/sources/virshcmdref/html/)

    ; Show the running VMs
    $ sudo /usr/local/bin/virsh list

    ; Show the xml template of a running vm
    $ sudo /usr/local/bin/virsh dumpxml <domain>

    ; Destroy a domain
    $ sudo /usr/local/bin/virsh destroy <domain>
