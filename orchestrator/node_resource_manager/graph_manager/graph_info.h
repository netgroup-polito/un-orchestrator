#ifndef GRAPH_INFO_H_
#define GRAPH_INFO_H_ 1

#pragma once

#include "lsi.h"
#include "../../network_controller/openflow_controller/controller.h"
#include "../../compute_controller/compute_controller.h"
#include "../graph/high_level_graph/high_level_graph.h"

#ifdef ENABLE_HARDWARE
#include "../../hardware_controller/hardware_controller.h"
#endif

class Controller;

class GraphInfo
{
/**
*	@brief: This class contains information on a specific graph.
**/

private:
	/**
	*	@brief: The Openflow controller the controls the LSI used to implement this graph
	*/
	Controller *controller;
	/**
	*	@brief: Data structure that describes the situation on the LSI used to implement this graph
	*/
	LSI *lsi;
	/**
	*	@brief: Compute controller used to handle the VNF of this graph
	*/
	ComputeController *computeController;
	/**
	*	@brief: Data structure that represents the graph as it has been received through the REST
	*		API (i.e., it includes VNFs, end-points, big switch, and it has not been split yet)
	*/
	highlevel::Graph *graph;

	//FIXME: PUT the following methods protected, and the GraphCreator as a friend?
public:
	GraphInfo();
	~GraphInfo();

	void setController(Controller *controller);
	void setLSI(LSI *lsi);
	void setComputeController(ComputeController *computeController);
	void setGraph(highlevel::Graph *graph);

	ComputeController *getComputeController();
	LSI *getLSI();
	Controller *getController();
	highlevel::Graph *getGraph();
};

#endif //GRAPH_INFO_H_
