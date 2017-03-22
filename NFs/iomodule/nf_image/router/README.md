## Iomodule-based router VNF

This file contains the instructions to build a iomodule network function working as router

### YAML Configuration Format

The following is an example of the configuration of a router:
```
[...]
- name: myrouter
  type: router
  config:
    interfaces:
      - name: Switch1
        ip: 10.0.1.254
        netmask: 255.255.255.0
        mac: "7e:ee:c2:01:01:01"

      - name: Switch2
        ip: 10.0.2.254
        netmask: 255.255.255.0
        mac: "7e:ee:c2:02:02:02"

      - name: Nat
        ip: 0.0.0.0
        netmask: 0.0.0.0
        mac: "7e:ee:c2:03:03:03"

    static_routes:
      - network: 130.192.0.0
        netmask: 255.255.0.0
        interface: Switch3
        next_hop: 10.2.2.254

      - network: 10.1.1.0
        netmask: 255.255.255.0
        interface: Switch4
        #If no next_hop set, route of type local

[...]
```

 - **interfaces**: defines local interfaces of the router.

For now, we cannot change dynamically (at run-time) the configuration.
