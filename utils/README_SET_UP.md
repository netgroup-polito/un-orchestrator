# How to automatically set up the Universal Node

In order to automatically compile and install all the Universal Node components 
on an **Ubuntu 14.04**, you have first to install `ansible` through the following 
command:

	$ sudo apt-get install ansible
	
Then, you have to run the script `deploy.yml` as follows:

	$ cd [un-orchestrator]/utils
	$ sudo ansible-playbook --ask-sudo-pass -i "localhost," -c local  deploy.yml --extra-vars "cores=1 docker=OFF native=OFF kvm=ON double_decker=OFF"
	
The previous command line includes some parameters that can be personalized before executing it:

  * `cores`: number of CPU cores to be used to compile the Universal Node components
  * `docker`: if set to `ON`, enables the Docker execution environment
  * `native`: if set to `ON`, enables the instantiation of native network functions
  * `kvm`: if set to `ON`, enables the QEMU/KVM execution environment
  * `double_decker`: if set to `ON`, enables the connection between the un-orchestrator and the Double Decker bus
  
**WARNING: the Docker execution environment is currently not installed by this script**
