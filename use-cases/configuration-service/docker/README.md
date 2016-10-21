### Creating the Docker image

You can create the Docker image of a function (e.g. NAT or DHCP) by launching the following command:

    sudo docker build --tag="nat" .

This will create the Docker image starting from the base image specified in `Dockerfile`; the new image is stored in the Docker default folder on your filesystem (localhost).

If you want to be more generic and publish the Docker image in a (public or private) repository, you can use the following command:

    sudo docker build --tag="localhost:5000/nat" .
    docker push localhost:5000/nat

This will register your VNF named `nat` in the local registry (given by the the string `localhost:5000`), which has to be up and running on localhost.


