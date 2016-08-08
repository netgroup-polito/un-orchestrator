# Docker-based VNF examples

This folder contains some examples of network functions implemented as Docker
containers.

## How to install Docker

In order to create your own VNF, you need to install the Docker environment by following the instructions
provided here:

	http://docs.docker.com/installation/

## How to create your VNFs

Please check individual README's in each sub-package.
Those files will give you the instruction to create the Docker image for the selected VNF.
Once you have that image, you can pass the link to the UN (by writing the appropriate entry in the name resolver configuration file) in order to instantiate it in your running environment.

## Important notes 

Please note is that each Docker container **MUST** have a script `start.sh`
in its root folder, which is invoked by the UN orchestrator to start the function
inside the container.
