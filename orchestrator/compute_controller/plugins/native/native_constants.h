#ifndef NATIVE_CONSTANTS_H_
#define NATIVE_CONSTANTS_H_ 1

/**
*	@brief: paths of the bash scripts used to manage native functions
*/
#define CHECK_NATIVE				"./compute_controller/plugins/native/scripts/checkNativeFunctions.sh"
#define PULL_AND_RUN_NATIVE_NF		"./compute_controller/plugins/native/scripts/pullAndRunNativeNF_ns.sh"
#define STOP_NATIVE_NF				"./compute_controller/plugins/native/scripts/stopNativeNF_ns.sh"
#define UPDATE_NATIVE_NF		"./compute_controller/plugins/native/scripts/updateNativeNF_ns.sh"

/*
 * @brief: paths and names for capabilities xml specification
 */
#define CAPABILITIES_FILE			"./compute_controller/plugins/native/Capabilities.xml"
#define CAPABILITIES_XSD			"./compute_controller/plugins/native/Capabilities.xsd"
#define CAP_CAPABILITY_ELEMENT		"capability"
#define CAP_NAME_ATTRIBUTE			"name"
#define CAP_TYPE_ATTRIBUTE			"type"
#define CAP_LOCATION_ATTRIBUTE		"location"
#define TYPE_SCRIPT					"script"
#define TYPE_EXECUTABLE				"executable"

#endif //NATIVE_CONSTANTS_H_
