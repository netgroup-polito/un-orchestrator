__author__ = 'Robert Szabo'

__copyright__ = "Copyright 2016, Ericsson Hungary Ltd."
__version__text__ = "virtualizer-utils"
__version__ = "0.1"

from virtualizer import *
from subprocess import call
import subprocess

logger = logging.getLogger(__name__)

__priority__ = 1

def start_process(args):
    try:
        p = subprocess.Popen(args,
            stdin = subprocess.PIPE,
            stdout = subprocess.PIPE,
            stderr = subprocess.PIPE,
            )
        out, err = p.communicate()
        return (p.returncode, out, err)
    except OSError:
        return (-1, None, None)

class Path:
    @staticmethod
    def split_tag_and_key_values(tag_with_key_values):
        if (tag_with_key_values.find("[") > 0) and (tag_with_key_values.find("]") > 0):
            tag = tag_with_key_values[0: tag_with_key_values.find("[")]
            key_values = tag_with_key_values[tag_with_key_values.find("[") + 1: tag_with_key_values.rfind("]")]
            kv = key_values.split("=")
            return tag, kv
        return tag_with_key_values, None

    @staticmethod
    def get_key_values_by_tag(path, tag):
        p = path.split("/")
        t = tag.split("/")
        for i in range(1, len(p)+1):
            _tag, _kv = Path.split_tag_and_key_values(p[-i])
            if (_tag == t[-1]) and (_kv is not None):
                match = True
                for j in range(1, min(len(p)+1-i,len(t))):
                    __tag, __kv = Path.split_tag_and_key_values(p[-i-j])
                    if __tag != t[-1-j]:
                        match = False
                        break
                if match:
                    return _kv[1]
        return None

    @staticmethod
    def has_tags(path, tags, at=None):
        """
        Check if path has tags (withough key and values
        :param path: string, path
        :param tags: pattern to check for
        :param at: int, position to check for (can be negative)
        :return: boolean, True if match; False otherwise
        """
        p = path.split("/")
        _path = ""
        for i in range(0, len(p)):
            l = p[i]
            if (l.find("[") > 0) and (l.find("]") > 0):
                attrib = l[0: l.find("[")]
                _path= _path + "/" + attrib
            else:
                _path= _path + l

        p = path.split('/')
        if at is not None:
            if len(p) > abs(at):
                return p[at] == path
            return False
        return path in p


class Bridge:
    @staticmethod
    def port_hash(s):
        ph = abs(hash(s)) % (10 ** 5)
        return str(ph)

    @staticmethod
    def delete_bridge_port(port, bridge='br0'):
        ret, out, err = start_process(["ovs-vsctl", '--if-exists', "del-port", bridge, port])
        if (ret != 0) or (err != ''):
            logger.error("delete_bidge_port failed: ret={ret}, out={out}, err={err}".format(ret=ret, out=out, err=err))
        else:
            logger.info("bridge port deleted {bridge}/{port}".format(bridge=bridge, port=port))
        pass


    @staticmethod
    def port2bridge_port(nf, port):
        ph = Bridge.port_hash(nf.id.get_as_text()+port.id.get_as_text())
        return ph

    @staticmethod
    def path2bridge_port_name(path):
        nf_id = Path.get_key_values_by_tag(path, 'NF_instances/node')
        if nf_id is None:
            return str(1)  # return default interface
        port_id = Path.get_key_values_by_tag(path, 'port')
        if port_id is None:
            logger.error("There is no port in path:{path}".format(path=path))
            raise ValueError("There is no port in path:{path}".format(path=path))
        ph = Bridge.port_hash(nf_id + port_id)
        return ph

    @staticmethod
    def path2bridge_port_id(path):
        port = Bridge.path2bridge_port_name(path)
        if port == '1':
            return int(1)

        ret, out, err = start_process(["./bin/get_interface_port.sh", port])
        if (ret != 0) or (err != ''):
            logger.error('get_interface_port.sh failed for path={path}; Ret= {ret}; out= {out}; err= {err}'.format(
                path=path, ret=ret, out=out, err=err))
            raise ValueError('get_interface_port.sh failed for path={path}; Ret= {ret}; out= {out}; err= {err}'.format(
                path=path, ret=ret, out=out, err=err))
        bridge_port = int(out)
        return bridge_port

    @staticmethod
    def get_port_id_from_path(path):
        p = path.split("/")
        l = p[-1]
        if (l.find("[") > 0) and (l.find("]") > 0):
            attrib = l[0: l.find("[")]
            if attrib != "port":
                return None
            key_value = l[l.find("[") + 1: l.rfind("]")]
            kv = key_value.split("=")
            return kv[1]
        return None
        pass

    @staticmethod
    def get_port_id_by_name(name):
        pass

class OpenFlow:
    @staticmethod
    def convert_action(a, port, ports_to_strip=()):
        res = ""
        if a is not None:
            action = a.split(':')
            if action[0] == 'pop_tag':
                res = "strip_vlan"
            if action[0] == "push_tag":
                res = "mod_vlan_vid:{tag}".format(tag=int(action[1],16))
            if port in ports_to_strip:  #TODO: ESCAPE BUG FIX
                res = "strip_vlan"
        return res

    @staticmethod
    def convert_match(m, port, ignore_vlan_ports=()):
        res = ""
        new_match = list()
        if m is not None:
            match = m.split(';')
            for i in range(0, len(match)):
                if port not in ignore_vlan_ports:
                    if 'dl_tag' in match[i]:
                        s = match[i].split('=')
                        new_match.append("dl_vlan={tag}".format(tag=int(s[1],16)))
                    else:
                        new_match.append(match[i])

            res = ','.join(new_match)
        return res


class FlowEntry:
    def __init__(self, v):
        self.id = v.id.get_as_text()
        self.match = ""
        self.action = ""
        self.history = list()

    def add_action(self, a):
        self.action = "actions=" + a

    def add_match(self, m):
        self.match = m

    def del_flows(self, history=-1, socket='br0'):
        if len(self.history)>0:
            h = self.history[history]
            arg = 'priority={priority},{match}'.format(
                priority= h['priority'],
                match=h['match'])
        ret, out, err = start_process(["ovs-ofctl", "--strict", "-t 2", "del-flows", socket, arg])
        if (ret != 0) or (err != ''):
            logger.error("del_flow failed {arg} with ret={ret}, out={out}, err={err}".format(arg=arg, ret=ret, out=out, err=err))
        else:
            logger.info("del_flow {match} at {bridge}".format(match=arg, bridge=socket))
        pass

    def add_flow(self, socket='br0'):
        match_action = self.match + "," + self.action
        h = None
        if len(self.history)>0:
            h = self.history[-1]
        global __priority__
        __priority__ += 1
        d = dict()
        d['match_action'] = match_action
        d['match'] = self.match
        d['action'] = self.action
        d['priority'] = __priority__
        self.history.append(d)
        command = "add-flow"
        # socket = "tcp:{host}:{port}".format(host = __SWITCH_IP__,  port = __SWITCH_PORT__)
        socket = 'br0'
        arg = 'priority={priority},idle_timeout=0,{match_action}'.format(
            priority= __priority__,
            match_action=match_action)
        logger.info("ovs-ofctl -t 2 {command} {socket} {arg}".format(command=command, socket=socket, arg=arg))
        call(["ovs-ofctl", "-t 2", command, socket, arg])
        if (h is not None):
            if h['match_action'] != match_action:
                self.del_flows(history=-2)
                pass

        pass

class FlowEntries:
    def __init__(self):
        self.items = dict()

    def get(self, id):
        if id in self.items.keys():
            return self.items[id]
        return None

    def add(self, fe):
        self.items[fe.id]= fe

    def pop(self, id):
        return self.items.pop(id)

    pass
