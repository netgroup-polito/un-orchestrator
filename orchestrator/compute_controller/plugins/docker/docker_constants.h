#ifndef DOCKER_CONSTANTS_H_
#define DOCKER_CONSTANTS_H_ 1

/**
*	@brief: paths of the bash scripts used to manage dockers
*/
#define CHECK_DOCKER			"./compute_controller/plugins/docker/scripts/checkDockerRun.sh"
#define PULL_AND_RUN_DOCKER_NF	"./compute_controller/plugins/docker/scripts/pullAndRunNF.sh"
#define STOP_DOCKER_NF			"./compute_controller/plugins/docker/scripts/stopNF.sh"
#define HOTPLUG_DOCKER_NF  "./compute_controller/plugins/docker/scripts/hotplugNF.sh"

#endif //DOCKER_CONSTANTS_H_
