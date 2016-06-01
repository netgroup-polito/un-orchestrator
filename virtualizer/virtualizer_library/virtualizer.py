#    Filename: virtualizer.py		 Created: 2016-03-18  19:56:13
#    This file was automatically created by a pyang plugin (PNC) developed at Ericsson Hungary Ltd., 2015
#    Authors: Robert Szabo, Balazs Miriszlai, Akos Recse, Raphael Vicente Rosa
#    Credits: Robert Szabo, Raphael Vicente Rosa, David Jocha, Janos Elek, Balazs Miriszlai, Akos Recse
#    Contact: Robert Szabo <robert.szabo@ericsson.com>
            
#    Yang file info:
#    Namespace: urn:unify:virtualizer
#    Prefix: virtualizer
#    Organization: ETH
#    Contact: Robert Szabo <robert.szabo@ericsson.com>
#    Description: V5.0: Common port configuration were added to the yang model from the metadata fields

__copyright__ = "Copyright 2015, Ericsson Hungary Ltd."
__license__ = "Apache License, Version 2.0"
__version__ = "2016-02-24"

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


from baseclasses import *


# YANG construct: grouping id-name
class GroupingId_name(Yang):
    def __init__(self, tag, parent=None, id=None, name=None):
        super(GroupingId_name, self).__init__(tag, parent)
        self._sorted_children = ["id", "name"]
        # yang construct: leaf
        self.id = StringLeaf("id", parent=self, value=id)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.name = StringLeaf("name", parent=self, value=name)
        """:type: StringLeaf"""


# YANG construct: grouping id-name-type
class GroupingId_name_type(GroupingId_name):
    def __init__(self, tag, parent=None, id=None, name=None, type=None):
        GroupingId_name.__init__(self, tag, parent, id, name)
        self._sorted_children = ["id", "name", "type"]
        # yang construct: leaf
        self.type = StringLeaf("type", parent=self, value=type)
        """:type: StringLeaf"""


# YANG construct: grouping metadata
class GroupingMetadata(Yang):
    def __init__(self, tag, parent=None):
        super(GroupingMetadata, self).__init__(tag, parent)
        self._sorted_children = ["metadata"]
        # yang construct: list
        self.metadata = ListYang("metadata", parent=self, type=MetadataMetadata)
        """:type: ListYang(MetadataMetadata)"""

    def add(self, item):
        return self.metadata.add(item)

    def remove(self, item):
        return self.metadata.remove(item)

    def __getitem__(self, key):
        return self.metadata[key]

    def __iter__(self):
        return self.metadata.itervalues()


# YANG construct: grouping link-resource
class GroupingLink_resource(Yang):
    def __init__(self, tag, parent=None, delay=None, bandwidth=None, cost=None):
        super(GroupingLink_resource, self).__init__(tag, parent)
        self._sorted_children = ["delay", "bandwidth", "cost"]
        # yang construct: leaf
        self.delay = StringLeaf("delay", parent=self, value=delay)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.bandwidth = StringLeaf("bandwidth", parent=self, value=bandwidth)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.cost = StringLeaf("cost", parent=self, value=cost)
        """:type: StringLeaf"""


# YANG construct: grouping l3-address
class GroupingL3_address(GroupingId_name):
    def __init__(self, tag, parent=None, id=None, name=None, configure=None, client=None, requested=None, provided=None):
        GroupingId_name.__init__(self, tag, parent, id, name)
        self._sorted_children = ["id", "name", "configure", "client", "requested", "provided"]
        # yang construct: leaf
        self.configure = StringLeaf("configure", parent=self, value=configure)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.client = StringLeaf("client", parent=self, value=client)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.requested = StringLeaf("requested", parent=self, value=requested)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.provided = StringLeaf("provided", parent=self, value=provided)
        """:type: StringLeaf"""


# YANG construct: grouping port
class GroupingPort(GroupingId_name, GroupingMetadata):
    def __init__(self, tag, parent=None, id=None, name=None, port_type=None, capability=None, sap=None, sap_data=None, control=None, addresses=None):
        GroupingId_name.__init__(self, tag, parent, id, name)
        GroupingMetadata.__init__(self, tag, parent)
        self._sorted_children = ["id", "name", "port_type", "capability", "sap", "sap_data", "control", "addresses", "metadata"]
        # yang construct: leaf
        self.port_type = StringLeaf("port_type", parent=self, value=port_type)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.capability = StringLeaf("capability", parent=self, value=capability)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.sap = StringLeaf("sap", parent=self, value=sap)
        """:type: StringLeaf"""
        # yang construct: container
        self.sap_data = None
        """:type: PortSap_data"""
        if sap_data is not None:
            self.sap_data = sap_data
        else:
            self.sap_data = PortSap_data(parent=self, tag="sap_data")
        # yang construct: container
        self.control = None
        """:type: PortControl"""
        if control is not None:
            self.control = control
        else:
            self.control = PortControl(parent=self, tag="control")
        # yang construct: container
        self.addresses = None
        """:type: PortAddresses"""
        if addresses is not None:
            self.addresses = addresses
        else:
            self.addresses = PortAddresses(parent=self, tag="addresses")


# YANG construct: grouping flowentry
class GroupingFlowentry(GroupingId_name):
    """The flowentry syntax will follow ovs-ofctrl string format. The UNIFY general tagging mechanism will be use like 'mpls'-> 'tag', i.e., push_tag:tag; pop_tag:tag..."""
    def __init__(self, tag, parent=None, id=None, name=None, priority=None, port=None, match=None, action=None, out=None, resources=None):
        GroupingId_name.__init__(self, tag, parent, id, name)
        self._sorted_children = ["id", "name", "priority", "port", "match", "action", "out", "resources"]
        # yang construct: leaf
        self.priority = StringLeaf("priority", parent=self, value=priority)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.port = Leafref("port", parent=self, value=port, mandatory=True)
        """:type: Leafref"""
        # yang construct: leaf
        self.match = StringLeaf("match", parent=self, value=match, mandatory=True)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.action = StringLeaf("action", parent=self, value=action, mandatory=True)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.out = Leafref("out", parent=self, value=out)
        """:type: Leafref"""
        # yang construct: container
        self.resources = None
        """:type: Link_resource"""
        if resources is not None:
            self.resources = resources
        else:
            self.resources = Link_resource(parent=self, tag="resources")


# YANG construct: grouping flowtable
class GroupingFlowtable(Yang):
    def __init__(self, tag, parent=None, flowtable=None):
        super(GroupingFlowtable, self).__init__(tag, parent)
        self._sorted_children = ["flowtable"]
        # yang construct: container
        self.flowtable = None
        """:type: FlowtableFlowtable"""
        if flowtable is not None:
            self.flowtable = flowtable
        else:
            self.flowtable = FlowtableFlowtable(parent=self, tag="flowtable")


# YANG construct: grouping link
class GroupingLink(GroupingId_name):
    def __init__(self, tag, parent=None, id=None, name=None, src=None, dst=None, resources=None):
        GroupingId_name.__init__(self, tag, parent, id, name)
        self._sorted_children = ["id", "name", "src", "dst", "resources"]
        # yang construct: leaf
        self.src = Leafref("src", parent=self, value=src)
        """:type: Leafref"""
        # yang construct: leaf
        self.dst = Leafref("dst", parent=self, value=dst)
        """:type: Leafref"""
        # yang construct: container
        self.resources = None
        """:type: Link_resource"""
        if resources is not None:
            self.resources = resources
        else:
            self.resources = Link_resource(parent=self, tag="resources")


# YANG construct: grouping links
class GroupingLinks(Yang):
    def __init__(self, tag, parent=None, links=None):
        super(GroupingLinks, self).__init__(tag, parent)
        self._sorted_children = ["links"]
        # yang construct: container
        self.links = None
        """:type: LinksLinks"""
        if links is not None:
            self.links = links
        else:
            self.links = LinksLinks(parent=self, tag="links")


# YANG construct: grouping software-resource
class GroupingSoftware_resource(Yang):
    def __init__(self, tag, parent=None, cpu=None, mem=None, storage=None, cost=None):
        super(GroupingSoftware_resource, self).__init__(tag, parent)
        self._sorted_children = ["cpu", "mem", "storage", "cost"]
        # yang construct: leaf
        self.cpu = StringLeaf("cpu", parent=self, value=cpu, mandatory=True)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.mem = StringLeaf("mem", parent=self, value=mem, mandatory=True)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.storage = StringLeaf("storage", parent=self, value=storage, mandatory=True)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.cost = StringLeaf("cost", parent=self, value=cost)
        """:type: StringLeaf"""


# YANG construct: grouping node
class GroupingNode(GroupingId_name_type, GroupingLinks, GroupingMetadata):
    """Any node: infrastructure or NFs"""
    def __init__(self, tag, parent=None, id=None, name=None, type=None, ports=None, links=None, resources=None):
        GroupingId_name_type.__init__(self, tag, parent, id, name, type)
        GroupingLinks.__init__(self, tag, parent, links)
        GroupingMetadata.__init__(self, tag, parent)
        self._sorted_children = ["id", "name", "type", "ports", "links", "resources", "metadata"]
        # yang construct: container
        self.ports = None
        """:type: NodePorts"""
        if ports is not None:
            self.ports = ports
        else:
            self.ports = NodePorts(parent=self, tag="ports")
        # yang construct: container
        self.resources = None
        """:type: Software_resource"""
        if resources is not None:
            self.resources = resources
        else:
            self.resources = Software_resource(parent=self, tag="resources")


# YANG construct: grouping nodes
class GroupingNodes(Yang):
    def __init__(self, tag, parent=None):
        super(GroupingNodes, self).__init__(tag, parent)
        self._sorted_children = ["node"]
        # yang construct: list
        self.node = ListYang("node", parent=self, type=Node)
        """:type: ListYang(Node)"""

    def add(self, item):
        return self.node.add(item)

    def remove(self, item):
        return self.node.remove(item)

    def __getitem__(self, key):
        return self.node[key]

    def __iter__(self):
        return self.node.itervalues()


# YANG construct: grouping infra-node
class GroupingInfra_node(GroupingNode, GroupingFlowtable):
    def __init__(self, tag, parent=None, id=None, name=None, type=None, ports=None, links=None, resources=None, NF_instances=None, capabilities=None, flowtable=None):
        GroupingNode.__init__(self, tag, parent, id, name, type, ports, links, resources)
        GroupingFlowtable.__init__(self, tag, parent, flowtable)
        self._sorted_children = ["id", "name", "type", "ports", "links", "resources", "metadata", "NF_instances", "capabilities", "flowtable"]
        # yang construct: container
        self.NF_instances = None
        """:type: Nodes"""
        if NF_instances is not None:
            self.NF_instances = NF_instances
        else:
            self.NF_instances = Nodes(parent=self, tag="NF_instances")
        # yang construct: container
        self.capabilities = None
        """:type: Infra_nodeCapabilities"""
        if capabilities is not None:
            self.capabilities = capabilities
        else:
            self.capabilities = Infra_nodeCapabilities(parent=self, tag="capabilities")


# YANG construct: list metadata
class MetadataMetadata(ListedYang):
    def __init__(self, tag="metadata", parent=None, key=None, value=None):
        ListedYang.__init__(self, "metadata", ["key"])
        self._sorted_children = ["key", "value"]
        # yang construct: leaf
        self.key = StringLeaf("key", parent=self, value=key, mandatory=True)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.value = StringLeaf("value", parent=self, value=value)
        """:type: StringLeaf"""


# YANG construct: list l3
class L3_address(ListedYang, GroupingL3_address):
    def __init__(self, tag="l3", parent=None, id=None, name=None, configure=None, client=None, requested=None, provided=None):
        ListedYang.__init__(self, "l3", ["id"])
        GroupingL3_address.__init__(self, tag, parent, id, name, configure, client, requested, provided)
        self._sorted_children = ["id", "name", "configure", "client", "requested", "provided"]


# YANG construct: list flowentry
class Flowentry(ListedYang, GroupingFlowentry):
    def __init__(self, tag="flowentry", parent=None, id=None, name=None, priority=None, port=None, match=None, action=None, out=None, resources=None):
        ListedYang.__init__(self, "flowentry", ["id"])
        GroupingFlowentry.__init__(self, tag, parent, id, name, priority, port, match, action, out, resources)
        self._sorted_children = ["id", "name", "priority", "port", "match", "action", "out", "resources"]


# YANG construct: list link
class Link(ListedYang, GroupingLink):
    def __init__(self, tag="link", parent=None, id=None, name=None, src=None, dst=None, resources=None):
        ListedYang.__init__(self, "link", ["id"])
        GroupingLink.__init__(self, tag, parent, id, name, src, dst, resources)
        self._sorted_children = ["id", "name", "src", "dst", "resources"]


# YANG construct: list port
class Port(ListedYang, GroupingPort):
    def __init__(self, tag="port", parent=None, id=None, name=None, port_type=None, capability=None, sap=None, sap_data=None, control=None, addresses=None):
        ListedYang.__init__(self, "port", ["id"])
        GroupingPort.__init__(self, tag, parent, id, name, port_type, capability, sap, sap_data, control, addresses)
        self._sorted_children = ["id", "name", "port_type", "capability", "sap", "sap_data", "control", "addresses", "metadata"]


# YANG construct: list node
class Node(ListedYang, GroupingNode):
    def __init__(self, tag="node", parent=None, id=None, name=None, type=None, ports=None, links=None, resources=None):
        ListedYang.__init__(self, "node", ["id"])
        GroupingNode.__init__(self, tag, parent, id, name, type, ports, links, resources)
        self._sorted_children = ["id", "name", "type", "ports", "links", "resources", "metadata"]


# YANG construct: list node
class Infra_node(ListedYang, GroupingInfra_node):
    def __init__(self, tag="node", parent=None, id=None, name=None, type=None, ports=None, links=None, resources=None, NF_instances=None, capabilities=None, flowtable=None):
        ListedYang.__init__(self, "node", ["id"])
        GroupingInfra_node.__init__(self, tag, parent, id, name, type, ports, links, resources, NF_instances, capabilities, flowtable)
        self._sorted_children = ["id", "name", "type", "ports", "links", "resources", "metadata", "NF_instances", "capabilities", "flowtable"]


# YANG construct: container sap_data
class PortSap_data(Yang):
    def __init__(self, tag="sap_data", parent=None, technology=None, resources=None):
        super(PortSap_data, self).__init__(tag, parent)
        self._sorted_children = ["technology", "resources"]
        # yang construct: leaf
        self.technology = StringLeaf("technology", parent=self, value=technology)
        """:type: StringLeaf"""
        # yang construct: container
        self.resources = None
        """:type: PortSap_dataResources"""
        if resources is not None:
            self.resources = resources
        else:
            self.resources = PortSap_dataResources(parent=self, tag="resources")


# YANG construct: container resources
class PortSap_dataResources(GroupingLink_resource):
    """Only used for domain boundary ports (port-sap type), where this is used to derive interconnection link characteristics."""
    def __init__(self, tag="resources", parent=None, delay=None, bandwidth=None, cost=None):
        GroupingLink_resource.__init__(self, tag, parent, delay, bandwidth, cost)
        self._sorted_children = ["delay", "bandwidth", "cost"]


# YANG construct: container control
class PortControl(Yang):
    """Used to connect this port to a UNIFY orchestrator's Cf-Or reference point. Support controller - orchestrator or orchestrator - controller connection establishment."""
    def __init__(self, tag="control", parent=None, controller=None, orchestrator=None):
        super(PortControl, self).__init__(tag, parent)
        self._sorted_children = ["controller", "orchestrator"]
        # yang construct: leaf
        self.controller = StringLeaf("controller", parent=self, value=controller)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.orchestrator = StringLeaf("orchestrator", parent=self, value=orchestrator)
        """:type: StringLeaf"""


# YANG construct: container addresses
class PortAddresses(Yang):
    def __init__(self, tag="addresses", parent=None, l2=None, l4=None):
        super(PortAddresses, self).__init__(tag, parent)
        self._sorted_children = ["l2", "l3", "l4"]
        # yang construct: list
        self.l3 = ListYang("l3", parent=self, type=L3_address)
        """:type: ListYang(L3_address)"""
        # yang construct: leaf
        self.l2 = StringLeaf("l2", parent=self, value=l2)
        """:type: StringLeaf"""
        # yang construct: leaf
        self.l4 = StringLeaf("l4", parent=self, value=l4)
        """:type: StringLeaf"""

    def add(self, item):
        return self.l3.add(item)

    def remove(self, item):
        return self.l3.remove(item)

    def __getitem__(self, key):
        return self.l3[key]

    def __iter__(self):
        return self.l3.itervalues()


# YANG construct: container resources
class Link_resource(GroupingLink_resource):
    def __init__(self, tag="resources", parent=None, delay=None, bandwidth=None, cost=None):
        GroupingLink_resource.__init__(self, tag, parent, delay, bandwidth, cost)
        self._sorted_children = ["delay", "bandwidth", "cost"]


# YANG construct: container flowtable
class FlowtableFlowtable(Yang):
    def __init__(self, tag="flowtable", parent=None):
        super(FlowtableFlowtable, self).__init__(tag, parent)
        self._sorted_children = ["flowentry"]
        # yang construct: list
        self.flowentry = ListYang("flowentry", parent=self, type=Flowentry)
        """:type: ListYang(Flowentry)"""

    def add(self, item):
        return self.flowentry.add(item)

    def remove(self, item):
        return self.flowentry.remove(item)

    def __getitem__(self, key):
        return self.flowentry[key]

    def __iter__(self):
        return self.flowentry.itervalues()


# YANG construct: container links
class LinksLinks(Yang):
    def __init__(self, tag="links", parent=None):
        super(LinksLinks, self).__init__(tag, parent)
        self._sorted_children = ["link"]
        # yang construct: list
        self.link = ListYang("link", parent=self, type=Link)
        """:type: ListYang(Link)"""

    def add(self, item):
        return self.link.add(item)

    def remove(self, item):
        return self.link.remove(item)

    def __getitem__(self, key):
        return self.link[key]

    def __iter__(self):
        return self.link.itervalues()


# YANG construct: container ports
class NodePorts(Yang):
    def __init__(self, tag="ports", parent=None):
        super(NodePorts, self).__init__(tag, parent)
        self._sorted_children = ["port"]
        # yang construct: list
        self.port = ListYang("port", parent=self, type=Port)
        """:type: ListYang(Port)"""

    def add(self, item):
        return self.port.add(item)

    def remove(self, item):
        return self.port.remove(item)

    def __getitem__(self, key):
        return self.port[key]

    def __iter__(self):
        return self.port.itervalues()


# YANG construct: container resources
class Software_resource(GroupingSoftware_resource):
    def __init__(self, tag="resources", parent=None, cpu=None, mem=None, storage=None, cost=None):
        GroupingSoftware_resource.__init__(self, tag, parent, cpu, mem, storage, cost)
        self._sorted_children = ["cpu", "mem", "storage", "cost"]


# YANG construct: container NF_instances
class Nodes(GroupingNodes):
    def __init__(self, tag="NF_instances", parent=None):
        GroupingNodes.__init__(self, tag, parent)
        self._sorted_children = ["node"]


# YANG construct: container capabilities
class Infra_nodeCapabilities(Yang):
    def __init__(self, tag="capabilities", parent=None, supported_NFs=None):
        super(Infra_nodeCapabilities, self).__init__(tag, parent)
        self._sorted_children = ["supported_NFs"]
        # yang construct: container
        self.supported_NFs = None
        """:type: Nodes"""
        if supported_NFs is not None:
            self.supported_NFs = supported_NFs
        else:
            self.supported_NFs = Nodes(parent=self, tag="supported_NFs")


# YANG construct: container virtualizer
class Virtualizer(GroupingId_name, GroupingLinks, GroupingMetadata):
    """Container for a single virtualizer"""
    def __init__(self, tag="virtualizer", parent=None, id=None, name=None, nodes=None, links=None, version='2016-02-24; compiled at 2016-03-18  19:56:13'):
        GroupingId_name.__init__(self, tag, parent, id, name)
        GroupingLinks.__init__(self, tag, parent, links)
        GroupingMetadata.__init__(self, tag, parent)
        self._sorted_children = ["id", "name", "nodes", "links", "metadata", "version"]
        # yang construct: container
        self.nodes = None
        """:type: VirtualizerNodes"""
        if nodes is not None:
            self.nodes = nodes
        else:
            self.nodes = VirtualizerNodes(parent=self, tag="nodes")
        # yang construct: leaf
        self.version = StringLeaf("version", parent=self, value=version)
        """:type: StringLeaf"""


# YANG construct: container nodes
class VirtualizerNodes(Yang):
    def __init__(self, tag="nodes", parent=None):
        super(VirtualizerNodes, self).__init__(tag, parent)
        self._sorted_children = ["node"]
        # yang construct: list
        self.node = ListYang("node", parent=self, type=Infra_node)
        """:type: ListYang(Infra_node)"""

    def add(self, item):
        return self.node.add(item)

    def remove(self, item):
        return self.node.remove(item)

    def __getitem__(self, key):
        return self.node[key]

    def __iter__(self):
        return self.node.itervalues()

