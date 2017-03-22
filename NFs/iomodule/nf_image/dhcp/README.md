## Iomodule-based dhcp VNF

This file contains the instructions to build a iomodule network function working as dhcp 
This module only can be connected to a single port.

### YAML Configuration Format

The following is an example of the configuration of a switch:
```
[...]
 - name: mydhcp
    type: dhcp
    config:
      netmask: 255.255.255.0
      addr_low: 192.168.1.100
      addr_high: 192.168.1.150
      dns: 8.8.8.8
      router: 192.168.1.1
      lease_time: 3600
      server_ip: 192.168.1.250
      server_mac: "b6:87:f8:5a:40:23"
[...]
```
 - **netmask**: mask of the network segment where the DHCP server is.
 - **addr_low**: first ip address that the server can assign.
 - **addr_high**: last ip address that the server can assign.
 - **dns**: DNS ip address that the server offers to the clients.
 - **router**: default gateway assigned to clients.
 - **lease_time**: default time that an address is leased to a client.
 - **server_ip**: IP address of the DHCP server.
 - **mac_ip**: MAC address of the DHCP server.

For now, we cannot change dynamically (at run-time) the configuration.
