This folder contains some scripts that automatize some annoying operations:

  * `start_ovs.sh`: start OvS and the associated data base. It is usefull at the
     boot of the UN
  * `cleaner.sh`: remove all the bridges created in OvS and the Docker containers.
     It is usefull in case the un-orchestrator crashes
  * `ovs_dump_flows.php` : has the same purpose of the command `ovs-ofctl dump-flows`, but uses interface names instead of interface numbers. Hence its output is more readable
