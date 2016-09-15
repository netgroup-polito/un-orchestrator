## Docker-based example ryu openflow controller

This file contains the instructions to build an example of Docker network function.

This is a container based on osrg/ryu docker container with additional python files for the elastic router control app.

### Creating the Docker image

You can create the Docker image with the network function by launching the following command:

    sudo docker build --tag="ctrl" .

Alternatively, this script can be used to build and push the image to `gitlab.testbed.se:5000/ctrl`
    
    ./build_ctrl_app

The name 'ctrl1' must be used for alignment with the name-resolver and nffg file.
This will create the Docker image starting from the base image specified in `Dockerfile`; the new image is stored in the Docker default folder on your filesystem (localhost).

The script start.sh is executed at startup and starts:
- ssh server
- ryu application
- DoubleDecker Proxy

IP address, Cf-Or interface address, are imported from the nffg.

### REST API
The ryu application starts a REST API wsgi server.
Port 5000 is used by the ryu application. This port needs to be exposed from the docker container if to reach this REST API from outside the container.
It is possible to trigger a scaling action via this interface like this (assuming port 5000 is exposed and mapped):

* `curl -X GET localhost:5000/scale/out` triggers a scale out action and scales the elastic router from a single data path to a separate data path container for each port.

* `curl -x GET localhost:5000/scale/in` triggers a scale in action and shifts again one data path container for all ports.

### SSH access
The deployed container starts an SSH server on port 22 inside the ctrl_app container.

SSH login can be done via `ssh root@localhost -p <mapped_port_to_22>` (to reach the ctrl container via exposed port)
SSH login credentials for the VNF containers:
* user: root
* password: root

To avoid password entering, a public key can be added to the `authorized_keys` file.
The fingerprint of the container changes on each build. SSH login complains about this because the fingerprint stored in  .ssh/known_hosts has changed after a new build. (This can be a man-in-the-middle attack).

To avoid this error, the  fingerprint of the container can be removed from .ssh/known_hosts or this command can be used to login via ssh: `ssh -o StrictHostKeyChecking=no root@localhost -p <mapped_port_to_22>`
