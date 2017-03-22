## Iomodule-based switch VNF

This file contains the instructions to build a iomodule network function working as switch 

### YAML Configuration Format

The following is an example of the configuration of a switch:
```
[...]
  - name: myswitch
    type: switch
    config:
      forwarding_table:
      - port: veth1
        mac: "b2:1b:34:5d:9b:2d"
      - port: veth2
        mac: "b2:1b:34:5d:9b:2e"
[...]
```

 - **forwarding _table**: defines static entries for the forwarding table of the switch. Please note that this configuration parameter is optional.

For now, we cannot change dynamically (at run-time) the configuration.
