# Iomodule VNF examples

This folder contains some examples of network functions implemented as iomodule functions.

## How to create your VNFs
Please check individual README's in each sub-package.
Those files will give you the instruction to create yaml file, named nf_description.yaml, and (optional) extra commands script, named extraCommands.sh.
All the file must be inserted in a tar.gz archive.

## Yaml file
The yaml file is composed of three sections: modules, links and external_interfaces:

```
# list of modules to be deployed
modules:
  - name: modulename
    type: moduletype
    config:
      # module configuration

# links between modules
links:
  - from: moduleName
    to: moduleName

# connection to network interfaces
external_interfaces:
  - module: moduleName
    iface: interface name
```

1. **modules**: This section contains the modules to be deployed.
The name and type are mandatory, while the configuration is optional and different for each kind of iomodules.
Please see the documentation of each single iomodule to get information about the configuration parameters.

2. **links**: These are the links between the different iomodules, "from" and "to" must correspond to the name of modules in the "modules" section.

3. **external_interfaces**:  The connection to the network interfaces are defined in this section. Module should be a module defined in the "modules" section and iface should be the name of the interface on the system.


## Command bash script
The extraCommands.sh file contains some commands to complete the configuration of interfaces used by iomodule. The commands are specific for each single iomodule.
This file will be automatically executed by UN to prepare the environment.
