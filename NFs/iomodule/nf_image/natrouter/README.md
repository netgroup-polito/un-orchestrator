## Iomodule-based NAT One-to-One router VNF

This file contains the instructions to build a iomodule network function working as nat linked to router. 
Please note that NAT MUST be attached to a router. 
Moreover: 
    The first port of the nat is always attached to the internal network.
    The second port of the nat is always attached to the public network.


### YAML Configuration Format

The following is an example of the configuration of a nat attached to a router:
```
[...]
   - name: Nat
    type: onetoonenat
    config:
      nat_entries:
      - internal_ip: 10.0.1.1
        external_ip: 130.192.1.1

      - internal_ip: 10.0.1.2
        external_ip: 130.192.1.2

  - name: Router
    type: router
    config:
      interfaces:
        - name: Switch1
          ip: 10.0.1.254
          netmask: 255.255.255.0
          mac: "7e:ee:c2:01:01:01"

        - name: Nat
          ip: 10.10.1.254
          netmask: 255.255.255.0
          mac: "7e:ee:c2:03:03:03"

      static_routes:
        - network: 0.0.0.0
          netmask: 0.0.0.0
          interface: Nat
          #next_hop: "0"
[...]
```

  * **nat_entries**: defines the ip mapping.
  * **internl_ip**: is the internal IP address.
  * **external_ip**: is the correspondent external IP address.

For now, we cannot change dynamically (at run-time) the configuration.
