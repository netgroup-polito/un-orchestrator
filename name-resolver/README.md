# Name-resolver

The Name Resolver is a module that returns a set of implementations for a given VNF. 
It is exploited by the un-orchestrator each time that a VNF must be started in order to 
translate the *abstract* name (e.g. `firewall`) into the proper suitable software image 
(e.g., `firewal_vmimage_abc`).

## Compile the name-resolver

We assume here that you already followed the general steps (e.g., required 
libraries) that are detailed in the general [../README_COMPILE.md](../README_COMPILE.md)
page.

The name resolver can be compiled together with the un-orchestrator. Please refer to [Compile the un-orchestrator and name-resolver](../orchestrator/README_COMPILE.md#compile-the-un-orchestrator-and-name-resolver)

# How to run the name-resolver

The full list of command line parameters for the name-resolver can be
retrieved by the following command:

	$ sudo ./name-resolver --h

Please refer to the help provided by the name-resolver itself in order to
understand how to use the different options.

Please check [config/example.xml](config/example.xml) to understand the configuration 
file required by the name-resolver. This file represents a database containing 
information on all the possible implementations for each available network function.
