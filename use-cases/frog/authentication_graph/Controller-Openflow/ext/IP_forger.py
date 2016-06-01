from pox.core import core
from threading import Lock
import thread
import socket
import struct
import json
import sys
import pox.openflow.libopenflow_01 as of
import pox.lib.packet as pkt
from pox.lib.util import str_to_bool
import Config

conf = Config.Configuration()
log = core.getLogger()

# First packet flag for each port
switchs_info = dict([])

lock = Lock()

# We don't want to flood immediately when a switch connects.
# Can be overriden on commandline.
_flood_delay = 0

class TCP_connection(object):
    IP_src = ""
    IP_dest = ""
    port_src = 0
    port_dst = 0

    # The class "constructor" - It's actually an initializer
    def __init__(self, IP_src, IP_dst, port_src, port_dst):
        self.IP_src = IP_src
        self.IP_dst = IP_dst
        self.port_src = port_src
        self.port_dst = port_dst
def make_tcp_connection(IP_src, IP_dst, port_src, port_dst):
    tcp_con = TCP_connection(IP_src, IP_dst, port_src, port_dst)
    return tcp_con

class host_infos(object):
    IP = ""
    MAC = ""
    switch_port = 0 #ingress user port
    flag_auth = 0

    # The class "constructor" - It's actually an initializer
    def __init__(self, IP, MAC, switch_port, flag_auth):
        self.IP = IP
        self.MAC = MAC
        self.switch_port = switch_port
        self.flag_auth = flag_auth
def make_host_infos(IP, MAC, switch_port, flag_auth):
    h = host_infos(IP, MAC, switch_port, flag_auth)
    return h

class port_infos(object):
    clients_info = None
    port_id = 0

    # The class "constructor" - It's actually an initializer
    def __init__(self,port_id):
        self.clients_info = dict([])
        self.port_id = port_id
def make_port_infos(port_id):
    p = port_infos(port_id)
    return p

class switch_infos(object):
    ports_info = None
    infos = None

    # The class "constructor" - It's actually an initializer
    def __init__(self,infos):
        self.ports_info = dict([])
        self.infos = infos
def make_switch_infos(infos):
    s = switch_infos(infos)
    return s


class server_infos(object):
    IP = ""
    MAC = ""
    switch_port = 0
    port = 0

    # The class "constructor" - It's actually an initializer
    def __init__(self, IP, MAC, switch_port, port):
        self.IP = IP
        self.MAC = MAC
        self.switch_port = switch_port
        self.port = port
def make_server_infos(IP, MAC, switch_port, port):
    h = server_infos(IP, MAC, switch_port, port)
    return h

def my_dpid_to_str (dpid, alwaysLong = False):
  """
  Convert a DPID from a long into into the canonical string form.
  """
  if type(dpid) is long or type(dpid) is int:
    # Not sure if this is right
    dpid = struct.pack('!Q', dpid)

  assert len(dpid) == 8

  r = ':'.join(['%02x' % (ord(x),) for x in dpid[2:]])

  if alwaysLong or dpid[0:2] != (b'\x00'*2):
    r += '|' + str(struct.unpack('!H', dpid[0:2])[0])

  return r
# Define a function for the thread
def my_socket(connection,*args):
    # IP address of SDN controller
    HOST = conf.my_infos.ip
    PORT = conf.my_infos.tcp_port#8888 # Arbitrary non-privileged port
    
    
    if Started().getStatus() == 0:
        log.info("IP_forger started for first time")
        Started().setStarted()
        
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # Saving socket in a global variable to maintain the connection in case of connection with openflow controller is lost
        handle = Handle()
        handle.setHandle(s)
        print ('Socket created')

        #Bind socket to local host and port
        try:
            log.info("Trying to do bind [host: " + HOST + ":" + str(PORT) + "]")
            s.bind((HOST, PORT))
        except socket.error as msg:
            print ('Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1])
            sys.exit()
    
        print ('Socket bind complete')

        #Start listening on socket
        s.listen(10)
        print ('Socket now listening')
    
    else:
        log.info("Openflow connection is back")
        handle = Handle()
        s = handle.getHandle()
        """
        log.info("Replug rules")
        # First rule: drop all traffic
        msg = of.ofp_flow_mod()
        msg.priority = 1
        msg.match.in_port = conf.my_infos.user_ports
        msg.actions.append(of.ofp_action_output(port=of.OFPP_NONE))
        self.connection.send(msg)
        # Second rule: normal process for DNS traffic
        msg = of.ofp_flow_mod()
        msg.priority = 100
        msg.match.in_port = conf.my_infos.user_ports
        msg.match.dl_type = 0x800
        msg.match.nw_proto = 17
        msg.match.tp_dst = 53
        msg.actions.append(of.ofp_action_output(port=of.OFPP_NORMAL))
        #msg.data = event.ofp
        self.connection.send(msg)
        # Third rule: normal process for ARP traffic
        msg = of.ofp_flow_mod()
        msg.priority = 100
        msg.match.in_port = conf.my_infos.user_ports
        msg.match.dl_type = 0x806
        msg.actions.append(of.ofp_action_output(port=of.OFPP_NORMAL))
        #msg.data = event.ofp
        self.connection.send(msg)
        # Fourth rule: normal process for DHCPDISCOVER
        msg = of.ofp_flow_mod()
        msg.priority = 100
        msg.match.in_port = conf.my_infos.user_ports
        msg.match.dl_type = 0x800
        msg.match.nw_proto = 17
        msg.match.nw_src = "0.0.0.0"
        msg.match.nw_dst = "255.255.255.255"
        msg.match.tp_src = 68
        msg.match.tp_dst = 67
        msg.actions.append(of.ofp_action_output(port=of.OFPP_NORMAL))
        #msg.data = event.ofp
        self.connection.send(msg)
        # Fifth rule: send the HTTP traffic to the controller
        msg = of.ofp_flow_mod()
        msg.priority = 1000
        msg.match.in_port = conf.my_infos.user_ports
        msg.match.dl_type = 0x800
        msg.match.nw_proto = 6
        msg.match.tp_dst = 80
        msg.actions.append(of.ofp_action_output(port=of.OFPP_CONTROLLER))
        msg.data = event.ofp
        self.connection.send(msg)
        """
        sys.exit(0)

    #now keep talking with the client
    while 1:
        #wait to accept a connection - blocking call
        conn, addr = s.accept()
        print ('Connected with ' + addr[0] + ':' + str(addr[1]))

        try:
            data = json.loads(conn.recv(4096).strip())
	    log.info(str(data))
        except Exception as e:
            print ("Exception while receiving message: ", e)

        # process the data
        if data['Msg_Type'] == "Auth_OK":
            # Authentication phase
            log.info("returned Auth_OK from Captive Portal")
            IP_address = data['IP_address']
            log.info("IP address is --> " + str(IP_address)+" !")
            for swich_id, switch in switchs_info.iteritems():
                log.info("For the switch " + str(swich_id)+ " ...")
                for port_id, port in switch.ports_info.iteritems():
                     
                    log.info("Port clients content is " + str(port.clients_info.iteritems()))
                    log.info("For the port " + str(port_id)+ " ...")
                    for MAC, client in port.clients_info.iteritems():
                        log.info("MAC is: " +str(MAC))
                        log.info("client.IP = "+str(client.IP)+" - IP_address = "+str(IP_address))
                        if client.IP == IP_address:
                            #clients_info[port] = client
                            log.info("For the MAC " + str(port_id)+ " sending data to CP...")
                            #time.sleep(0.5)
                            #Send some data to remote server
                            data = {'Msg_Type':'Auth_OK_Resp', 'Msg_Content':'','IP_address':switch.infos.sock.getpeername()[0],'MAC':my_dpid_to_str(switch.infos.dpid),'user_MAC':MAC.toStr(),'user_port':str(switch.infos.ports[client.switch_port].name)}
                            print(json.dumps(data))
                            try :
                                #Set the whole string
                                conn.sendall(json.dumps(data)+"\r\n")
                            except socket.error:
                                #Send failed
                                print ('Send failed')
                                sys.exit()
                            print ('Message 1 send successfully')
        elif data['Msg_Type'] == 'Deploy_OK':
            log.warning("returned Deploy_OK from Captive Portal")
            cptv = server_infos(conf.captive_portal.ip, conf.captive_portal.mac, conf.captive_portal.switch_port,conf.captive_portal.tcp_port)
            IP_address = data['IP_address']
            for swich_id, switch in switchs_info.iteritems():
                for port_id, port in switch.ports_info.iteritems():
                    for MAC, client in port.clients_info.iteritems():
                        if client.IP == IP_address:
                            lock.acquire()
                            try:
                                log.warning("Setting flag_auth to 1")
                                client.flag_auth = 1
                            finally:
                                lock.release() # release lock, no matter what
                            log.info("Deploy OK received")
                            log.info("Reconfigure the switch")

                            #1) del "port" flows
                            connection.send( of.ofp_flow_mod( command = of.OFPFC_DELETE, match=of.ofp_match( in_port = port_id, dl_src = client.MAC)))
                            #2) new rule to allow authenticated user to reach captive_portal
                            msg = of.ofp_flow_mod()
                            msg.priority = 10000
                            msg.match.dl_type = 0x800
                            msg.match.dl_src = client.MAC
                            #msg.match.nw_proto = 6
                            msg.match.nw_dst = cptv.IP
                            msg.match.in_port = client.switch_port
                            msg.actions.append(of.ofp_action_output(port=cptv.switch_port))
                            connection.send(msg)
                            # msg = of.ofp_flow_mod()
                            # msg.priority = 10000
                            # msg.match.dl_type = 0x806
                            # msg.match.dl_src = client.MAC
                            # msg.match.arp_tpa = cptv.IP
                            # msg.match.in_port = client.switch_port
                            # msg.actions.append(of.ofp_action_output(port=cptv.switch_port))
                            # connection.send(msg)

                            data = {'Msg_Type':'Deploy_OK_Resp', 'Msg_Content':'' }
                            print(json.dumps(data))
                            try :
                                #Set the whole string
                                conn.sendall(json.dumps(data)+"\r\n")
                            except socket.error:
                                #Send failed
                                print ('Send failed')
                                sys.exit()
                            print ('Message 2 send successfully')

        elif data['Msg_Type'] == 'Delete_OK':
            log.warning("returned Delete_OK from Captive Portal")
            cptv = server_infos(conf.captive_portal.ip, conf.captive_portal.mac, conf.captive_portal.switch_port,
                                conf.captive_portal.tcp_port)
            IP_address = data['IP_address']
            for swich_id, switch in switchs_info.iteritems():
                for port_id, port in switch.ports_info.iteritems():
                    for MAC, client in port.clients_info.iteritems():
                        if client.IP == IP_address:
                            lock.acquire()
                            try:
                                log.warning("Setting flag_auth to 0")
                                client.flag_auth = 0
                            finally:
                                lock.release()  # release lock, no matter what
                            log.info("Delete OK received")
                            log.info("Reconfigure the switch")

                            # 1) del "port" flows
                            connection.send(
                                of.ofp_flow_mod(command=of.OFPFC_DELETE, match=of.ofp_match(in_port=port_id, dl_src=client.MAC)))

                            data = {'Msg_Type': 'Delete_OK_Resp', 'Msg_Content': ''}
                            print(json.dumps(data))
                            try:
                                # Set the whole string
                                conn.sendall(json.dumps(data) + "\r\n")
                            except socket.error:
                                # Send failed
                                print ('Send failed')
                                sys.exit()
                            print ('Message 2 send successfully')




        else:
            log.warning("unknown message returned from Captive Portal")


    s.close()

class IPForger (object):
  """
  When we see a new packet create a new flow in the switch with the MAC:IP:tcp_
  port destination of the Captive Portal (these values are given as input) and
  another new flow that for the packets that come from the Captive Portal and
  have IP:port destination equal to IP:port source of the initial packet forge
  the source headers with the one we substituted before.


  In short, our algorithm looks like this:

  For each packet from the switch:
  1) Save in a table IP:port source/destination
  2) Add 2 new flows to the OVS

  """
  def __init__ (self, connection, transparent):
    # Switch we'll be adding L2 learning switch capabilities to
    self.connection = connection
    self.transparent = transparent

    # Our table
    self.TCP_connections = []

    # We want to hear PacketIn messages, so we listen
    # to the connection
    connection.addListeners(self)

    # We just use this to know when to log a helpful message
    self.hold_down_expired = _flood_delay == 0

    #log.info("Initializing LearningSwitch, transparent=%s",
    #          str(self.transparent))

  def _handle_PacketIn (self, event):
    """
    Handle packet in messages from the switch to implement above algorithm.
    """#52:54:00:79:95:26 26:6a:fb:3c:f8:fd
    cptv = server_infos(conf.captive_portal.ip,conf.captive_portal.mac,conf.captive_portal.switch_port,conf.captive_portal.tcp_port)#"130.192.225.168","52:54:00:79:95:26",5,8085)
    #cptv = server_infos("130.192.225.156","88:51:fb:42:2b:f8",5,8080)

    if self.connection.dpid not in switchs_info:
        lock.acquire()
        try:
            dpi = DPI()
            dpi.setDPI(self.connection.dpid)
            # add a new switch to our infos
            switchs_info[self.connection.dpid] = switch_infos(self.connection)
        finally:
            lock.release() # release lock, no matter what

    switch =  switchs_info[self.connection.dpid]
    if event.port not in switch.ports_info:
        lock.acquire()
        try:
            #print (dpid_to_str(self.connection.dpid))
            #print self.connection.sock.getpeername()[0]
            switch.ports_info[event.port] = port_infos(event.port)
        finally:
            lock.release() # release lock, no matter what

        #if event.port == conf.my_infos.my_default_GRE_tunnel:
        is_user_port = True
        log.info("Comparing the ports speaking " + str(event.port))
        for not_user_port in conf.my_infos.not_user_ports:
            if str(event.port) == str(not_user_port):
                is_user_port = False
                break

        if not is_user_port:
            log.info("Event port is " + str(event.port))
            log.info("First for the ethernet interface")
            # First rule: allow all traffic
            self.connection.send( of.ofp_flow_mod( action=of.ofp_action_output(port=of.OFPP_NORMAL),
                                       priority=1,
                                       match=of.ofp_match( in_port = event.port)))
        else:
            """
            Packet from user
            """
            log.info("First packet from user")
            # First rule: drop all traffic
            msg = of.ofp_flow_mod()
            msg.priority = 1
            msg.match.in_port = event.port
            msg.actions.append(of.ofp_action_output(port=of.OFPP_NONE))
            self.connection.send(msg)
            # Second rule: normal process for DNS traffic
            msg = of.ofp_flow_mod()
            msg.priority = 100
            msg.match.in_port = event.port
            msg.match.dl_type = 0x800
            msg.match.nw_proto = 17
            msg.match.tp_dst = 53
            msg.actions.append(of.ofp_action_output(port=of.OFPP_NORMAL))
            #msg.data = event.ofp
            self.connection.send(msg)
            # Third rule: normal process for ARP traffic
            msg = of.ofp_flow_mod()
            msg.priority = 100
            msg.match.in_port = event.port
            msg.match.dl_type = 0x806
            msg.actions.append(of.ofp_action_output(port=of.OFPP_NORMAL))
            #msg.data = event.ofp
            self.connection.send(msg)
            # Fourth rule: normal process for DHCPDISCOVER
            msg = of.ofp_flow_mod()
            msg.priority = 100
            msg.match.in_port = event.port
            msg.match.dl_type = 0x800
            msg.match.nw_proto = 17
            msg.match.nw_src = "0.0.0.0"
            msg.match.nw_dst = "255.255.255.255"
            msg.match.tp_src = 68
            msg.match.tp_dst = 67
            msg.actions.append(of.ofp_action_output(port=of.OFPP_NORMAL))
            #msg.data = event.ofp
            self.connection.send(msg)
            # Fifth rule: send the HTTP traffic to the controller
            msg = of.ofp_flow_mod()
            msg.priority = 1000
            msg.match.in_port = event.port
            msg.match.dl_type = 0x800
            msg.match.nw_proto = 6
            msg.match.tp_dst = 80
            msg.actions.append(of.ofp_action_output(port=of.OFPP_CONTROLLER))
            msg.data = event.ofp
            self.connection.send(msg)

    packet = event.parsed
    if not packet.parsed:
      log.info("ignoring unparsed packet")
      return

    client_list = switch.ports_info[event.port].clients_info
    log.info("LOOP Thread id: "+str(thread.get_ident() ))
    log.debug("packet.type: "+ str(packet.type) +" || pkt.ethernet.IP_TYPE: "+ str(pkt.ethernet.IP_TYPE))
    if packet.type == pkt.ethernet.IP_TYPE:
        ipv4_hdr = packet.payload
        log.debug("ipv4_hdr.protocol: "+ str(ipv4_hdr.protocol) +" || pkt.ipv4.TCP_PROTOCOL: "+ str(pkt.ipv4.TCP_PROTOCOL))
        if ipv4_hdr.protocol == pkt.ipv4.TCP_PROTOCOL:
            tcp_hdr = ipv4_hdr.payload
            log.info("tcp_hdr.dstport: "+ str(tcp_hdr.dstport) +" || DestPort: || 80")
            if tcp_hdr.dstport == 80:
                log.debug("packet.src: "+ str(packet.src))

                if packet.src not in client_list:
                    log.info("packet.src is not in client_list")
                    lock.acquire()
                    try:
                        log.info("save host info: IP=" + str(ipv4_hdr.srcip) + ", MAC=" + str(packet.src) + ", INGRESS_SWITCH_PORT=" + str(switch.infos.ports[event.port].name))
                        client_list[packet.src] = host_infos(ipv4_hdr.srcip,packet.src,event.port,0)
                    finally:
                        lock.release() # release lock, no matter what
                else:
                    log.debug("PACKET_IN INFOs: user MAC: "+ str(packet.src)+", user IP: "+ str(ipv4_hdr.srcip))
                    user_infos = client_list[packet.src]
                    user_ip = user_infos.IP
                    log.debug("Local informations: user MAC: "+ str(packet.src)+", user IP: "+ str(user_ip))
                    if user_ip != ipv4_hdr.srcip:
                        log.debug("Updating client IP informations")
                        user_infos.IP = ipv4_hdr.srcip

                client = client_list[packet.src]
                if (client.flag_auth == 0):
                    log.info("installing new flow to set destination IP to captive portal IP")
                    msg = of.ofp_flow_mod()
                    msg.priority = 10000
                    msg.idle_timeout=30
                    msg.match.dl_type = 0x800
                    msg.match.dl_src = client.MAC
                    msg.match.nw_proto = 6
                    #Adding the match on the TCP source port of the user, in order to genererate a PACKET_IN for every
                    #new TCP connection
                    msg.match.tp_src = tcp_hdr.srcport
                    msg.match.tp_dst = 80
                    msg.match.nw_dst = ipv4_hdr.dstip
                    msg.match.in_port = event.port
                    msg.actions.append(of.ofp_action_dl_addr(of.OFPAT_SET_DL_DST,cptv.MAC))
                    msg.actions.append(of.ofp_action_nw_addr(of.OFPAT_SET_NW_DST,cptv.IP))
                    msg.actions.append(of.ofp_action_tp_port(of.OFPAT_SET_TP_DST,cptv.port))
                    msg.actions.append(of.ofp_action_output(port=cptv.switch_port))
                    #msg.data = event.ofp
                    self.connection.send(msg)
                    log.info("installing new flow to set return path")
                    msg = of.ofp_flow_mod()
                    msg.priority = 10000
                    msg.idle_timeout=30
                    msg.match.dl_type = 0x800
                    msg.match.nw_proto = 6
                    msg.match.tp_dst = tcp_hdr.srcport
                    msg.match.nw_dst = ipv4_hdr.srcip
                    msg.match.nw_src = cptv.IP
                    msg.actions.append(of.ofp_action_nw_addr(of.OFPAT_SET_NW_SRC,ipv4_hdr.dstip))
                    msg.actions.append(of.ofp_action_tp_port(of.OFPAT_SET_TP_SRC,tcp_hdr.dstport))
                    msg.actions.append(of.ofp_action_output(port=event.port))
                    msg.data = event.ofp
                    self.connection.send(msg)
                '''
                else:
                    log.warning("Auth user")
                    self.connection.send( of.ofp_flow_mod( command = of.OFPFC_DELETE,
                                        match=of.ofp_match( dl_type=0x800,
                                                            nw_dst=client.IP,
                                                            nw_src=cptv.IP)))
                    #2) add new "port" flows
                    self.connection.send( of.ofp_flow_mod( action=of.ofp_action_output(port = conf.my_infos.my_default_GRE_tunnel),#5),#GRE tunnel port
                                                       priority=65000,
                                                       match=of.ofp_match( in_port = event.port, dl_src = client.MAC)))
                    #self.connection.send( of.ofp_flow_mod( action=of.ofp_action_output(port = event.port),
                     #                                  priority=65000,
                      #                                 match=of.ofp_match( dl_type=0x800,
                       #                                                    nw_dst=client.IP,
                        #                                                   in_port = conf.my_infos.my_default_GRE_tunnel)))#5)))#GRE tunnel port
                    #self.connection.send( of.ofp_flow_mod( action=of.ofp_action_output(port=of.OFPP_NORMAL),
                     #                      priority=1,
                      #                     match=of.ofp_match( in_port = conf.my_infos.my_default_GRE_tunnel)))
                      '''
class Singleton(type):
    _instances = {}
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


class Started( object):
    __metaclass__ = Singleton
    def __init__(self):
        self.started = 0

    def getStatus(self):
        return self.started

    def setStarted(self):
        self.started = 1

class Handle( object):
    __metaclass__ = Singleton
    def __init__(self):
        self.handle = None

    def getHandle(self):
        return self.handle

    def setHandle(self, handle):
        self.handle = handle

class DPI( object):
    __metaclass__ = Singleton
    def __init__(self):
        self.dpi = None

    def getDPI(self):
        return self.dpi

    def setDPI(self, dpi):
        self.dpi = dpi

class IP_forger (object):
    """
      Waits for OpenFlow switches to connect and makes them learning switches.
    """
    def __init__ (self, transparent):
        core.openflow.addListeners(self)
        self.transparent = transparent


    def _handle_ConnectionUp (self, event):
        log.info("Connection %s" % (event.connection,))
        IPForger(event.connection, self.transparent)
        try:
            t_id = thread.start_new_thread( my_socket, (event.connection,1) )
            log.info("Thread started")
            log.info("Thread id :"+str(t_id))

        except:
           print ("Error: unable to start socket thread")

    def _handle_ConnectionDown(self, event):
        dpi = DPI()
        down_dpi = dpi.getDPI()
        del switchs_info[down_dpi]
        #switchs_info.remove(down_dpi)
        #switchs_info[down_dpi]  = dict([])
        log.warning("Lost openflow connection")

def launch (transparent=False, hold_down=_flood_delay):

  try:
    global _flood_delay
    _flood_delay = int(str(hold_down), 10)
    assert _flood_delay >= 0
  except:
    raise RuntimeError("Expected hold-down to be a number")

  core.registerNew(IP_forger, str_to_bool(transparent))
