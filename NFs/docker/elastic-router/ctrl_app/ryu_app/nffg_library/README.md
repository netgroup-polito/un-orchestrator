# NF-FG Library

This library models the NF-FG used by the frog-orchestrator.
The NF-FG is similar to the one proposed in the UNIFY project, but:
  * it captures only the communication from a component to its underlying counterpart, e.g., from an orchestrator to a compute node (downstream communication flow)
  * it looks a little bit more human readable.

This NF-FG model is structured with the following main sections:
  * A list of VNFs
  * A list of Endpoints
  * A "big-switch" to create the interconnections between components (list of flow-rules and actions, similar to OpenFlow 1.0)
 
This library is Python 2/3 compatible and is referenced as a submodule in all projects that use it.
