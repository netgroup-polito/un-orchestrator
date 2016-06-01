## Docker-based example autoTPG

This file contains the instructions to build an example of Docker network function.

This is a container based on a ubuntu container with autoTPG installed

### Creating the Docker image

You can create the Docker image with the network function by launching the following command:

    sudo docker build --tag="autotpg" .
    
The container should be started so the autoTPG controller is reachable from the VNFs deployed on external nodes.
The autoTPG container should bind to the host the (default) port where it listens for OpenFlow connections.

    sudo docker run -it -p 6633:6633 autotpg
