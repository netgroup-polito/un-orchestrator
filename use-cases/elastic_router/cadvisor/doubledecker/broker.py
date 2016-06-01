__license__ = """
  Copyright (c) 2015 Pontus Sköldström, Bertrand Pechenot

  This file is part of libdd, the DoubleDecker hierarchical
  messaging system DoubleDecker is free software; you can
  redistribute it and/or modify it under the terms of the GNU Lesser
  General Public License (LGPL) version 2.1 as published by the Free
  Software Foundation.

  As a special exception, the Authors give you permission to link this
  library with independent modules to produce an executable,
  regardless of the license terms of these independent modules, and to
  copy and distribute the resulting executable under terms of your
  choice, provided that you also meet, for each linked independent
  module, the terms and conditions of the license of that module. An
  independent module is a module which is not derived from or based on
  this library.  If you modify this library, you must extend this
  exception to your version of the library.  DoubleDecker is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
  License for more details.  You should have received a copy of the
  GNU Lesser General Public License along with this program.  If not,
  see <http://www.gnu.org/licenses/>.
"""

__author__ = 'Bertrand Pechenot'
__email__ = 'berpec@acreo.se'

import pprint
import abc
import json
import sys
import logging

import zmq
import zmq.eventloop.ioloop
import zmq.eventloop.zmqstream
import nacl.utils
import nacl.public
import nacl.encoding

from . import proto as DD
from . import trie as trie

from pprint import pprint


class Broker(object, metaclass=abc.ABCMeta):
    """
    Base broker class
    :param name:
    :param routerurl:
    :param dealerurl:
    :param scope:
    :param pubsub:
    """

    def __init__(self, name, routerurl, dealerurl, scope):
        self.ctx = zmq.Context()
        self.routerurl = routerurl
        self.router = self.ctx.socket(zmq.ROUTER)
        self.dealerurl = dealerurl
        self.dealer = self.ctx.socket(zmq.DEALER)
        self.local_cli = dict()
        self.reverse_local_cli = dict()
        self.local_cli_timeout = dict()
        self.registered_client = set()
        self.hashes = dict()
        self.dist_cli = dict()
        self.local_br = dict()
        self.tenants = list()
        self.state = DD.S_ROOT
        self.name = name
        self.rname = ("|%s|" % name.decode()).encode()
        self.timeout = 0
        self.pub_southub = True
        self.random_number = 0
        self.subscriptions = dict()
        self.reverse_sub = dict()
        self.topics_trie = trie.CharTrie()
        self.pubkey = ''
        self.privkey = ''
        self.scope = scope

        self.pub_northurl = None
        self.pub_southurl = None
        self.sub_northurl = None
        self.sub_southurl = None

        # pubsub on dealer direction
        self.pub_north_sock = self.ctx.socket(zmq.XPUB)
        self.sub_north_sock = self.ctx.socket(zmq.XSUB)
        # pubsub on router direction
        self.pub_south_sock = self.ctx.socket(zmq.XPUB)
        self.sub_south_sock = self.ctx.socket(zmq.XSUB)

        self.topic_north = dict()
        self.topic_south = dict()

        self.IOLoop = zmq.eventloop.ioloop.IOLoop.instance()
        if sys.stdout.isatty():
            self.IOLoop.add_handler(sys.stdin.fileno(),self.on_stdin,self.IOLoop.READ)

        self.router_stream = zmq.eventloop.zmqstream.ZMQStream(self.router)
        self.dealer_stream = zmq.eventloop.zmqstream.ZMQStream(self.dealer, self.IOLoop)
        self.pub_north_stream = zmq.eventloop.zmqstream.ZMQStream(self.pub_north_sock, self.IOLoop)
        self.sub_north_stream = zmq.eventloop.zmqstream.ZMQStream(self.sub_north_sock, self.IOLoop)
        self.pub_south_stream = zmq.eventloop.zmqstream.ZMQStream(self.pub_south_sock, self.IOLoop)
        self.sub_south_stream = zmq.eventloop.zmqstream.ZMQStream(self.sub_south_sock, self.IOLoop)

        self.register_loop = zmq.eventloop.ioloop.PeriodicCallback(self.ask_registration, 1000)
        self.cli_timeout_loop = zmq.eventloop.ioloop.PeriodicCallback(self.check_cli_timeout, 1000)
        self.br_timeout_loop = zmq.eventloop.ioloop.PeriodicCallback(self.check_br_timeout, 1000)
        self.heartbeat = zmq.eventloop.ioloop.PeriodicCallback(self.ping, 1000)

        self.dealer_cmds = {DD.bCMD_REGOK: self.reg_ok,
                            DD.bCMD_NODST: self.dest_invalid,
                            DD.bCMD_FORWARD: self.forward,
                            DD.bCMD_PONG: self.dummy,
                            DD.bCMD_FORWARDPT: self.forwardpt}
#                            DD.bCMD_NAMEREP: self.namereply}

        self.router_cmds = {DD.bCMD_ADDLCL: self.add_local_cli,
                            DD.bCMD_ADDDCL: self.add_dist_cli,
                            DD.bCMD_ADDBR: self.add_local_br,
                            DD.bCMD_UNREG: self.unreg_cli,
                            DD.bCMD_UNREGDCLI: self.unreg_dist_cli,
                            DD.bCMD_UNREGBR: self.unreg_br,
                            DD.bCMD_FORWARD: self.forward,
                            DD.bCMD_PING: self.pong,
                            DD.bCMD_SEND: self.send,
                            DD.bCMD_CHALLOK: self.reg_local_cli,
                            DD.bCMD_PUB: self.pub,
                            DD.bCMD_SUB: self.sub,
                            DD.bCMD_UNSUB: self.unsub,
                            DD.bCMD_FORWARDPT: self.forwardpt,
                            DD.bCMD_SENDPT: self.sendpt}

#                            DD.bCMD_NAMEREQ: self.namerequest}

        self.router.setsockopt(zmq.IDENTITY, self.name)

        try:
            for url in self.routerurl.split(','):
                #logging.debug("routerurl split %s" % url)
                self.router.bind(url)
        except zmq.error.ZMQError as e:
            logging.critical('An error happened durind the binding of' % self.routerurl)
            logging.critical('Error says :' % e.strerror)
            self.shutdown()
            return

        self.router_stream.on_recv(self.on_router_msg, self.IOLoop)

        self.dealer.setsockopt(zmq.IDENTITY, self.name)
        if self.dealerurl != '':
            self.dealer.connect(self.dealerurl)
            self.dealer_stream.on_recv(self.on_dealer_msg)

            self.register_loop.start()

        self.cli_timeout_loop.start()
        self.br_timeout_loop.start()

        self.pub_southurl = ''
        self.sub_southurl = ''
        url = ''
        if self.pub_southub:
            #logging.debug("looking for urls in %s" % self.routerurl.split(','))
            for url in self.routerurl.split(','):
                if 'tcp' in url:
                    port = url.split(':')[-1]
                    #logging.debug("Port number: %s" % port)
                    pubport = int(port) + 1
                    subport = int(port) + 2
                    self.pub_southurl = url.replace(port, "%d" % pubport)
                    self.sub_southurl = url.replace(port, "%d" % subport)
                    #logging.debug("pub and sub strings: %s %s" % (self.pub_southurl, self.sub_southurl))
                elif 'ipc' in url:
                    logging.debug("IPC: %s" % url.split(':')[-1])
                else:
                    logging.warning("routerurl is not tcp or ipc, pub/sub not supported!")

        if self.pub_southurl and self.sub_southurl:
            #logging.debug("binding client Publish/subscribe sockets (pubS/subS)")
            try:
                for url in self.pub_southurl.split(','):
                    self.pub_south_sock.bind(url)
            except zmq.error.ZMQError as e:
                logging.critical('PublisherS: error happened during the binding of %s' % url)
                logging.critical('Error says : %s' % e.strerror)
                self.shutdown()
                return
            self.pub_south_stream.on_recv(self.on_pub_south_msg, self.IOLoop)

            try:
                for url in self.sub_southurl.split(','):
                    self.sub_south_sock.bind(url)

                self.sub_south_stream.on_recv(self.on_sub_south_msg)
            except zmq.error.ZMQError as e:
                logging.critical('SubscriberS: error happened during the binding of %s' % url)
                logging.critical('Error says : %s' % e.strerror)
                self.shutdown()
                return

        logging.info("Configured: name = %s, Router = %s" %
                     (self.name, self.routerurl))
        if self.dealerurl == '':
            logging.info('configured as root of the architecture')
        else:
            logging.info('dealer : %s' % self.dealerurl)

        # Configure scope
        try:
            self.scopearray = self.scope.split('/')
            logging.info('Scope: %s' % self.scope)
        except:
            logging.critical("Error: The scope should be given as '1/2/3'")
            self.shutdown()
            return

    def start(self):
        """
        Start the broker

        """
        try:
            self.IOLoop.start()
        except:
            self.shutdown()

    #
    # Definition of the behavior in case of message received from the broker
    #
    def on_dealer_msg(self, msg):
        """
        Function called automatically when a message is received on the
        socket connected to the broker.
        :param msg: Message received from the broker
        :return: None
        """
        self.timeout = 0
        if msg.pop(0) != DD.bPROTO_VERSION:
            logging.warning('different protocols in use, message from the broker')
            return
        cmd = msg.pop(0)
        if cmd in self.dealer_cmds:
            self.dealer_cmds[cmd](args=msg)
        else:
            logging.warning('Ununderstood command from broker : %d'
                            % int.from_bytes(cmd, 'little'))

    def reg_ok(self, args):
        """
        Function called if the command received is REGOK.
        At this point the broker can be considered registered to its dealer
        :param args: Frames following the command in the original message
        :return: None
        """
        self.state = DD.S_REGISTERED
        self.register_loop.stop()
        self.heartbeat.start()
        for ident in self.local_cli:
            client_name = '.'.join([self.local_cli[ident][1], self.local_cli[ident][0].decode()]).encode()
            self.add_client_up(client_name, 0)
        for name in self.dist_cli:
            self.add_client_up(name, int(self.dist_cli[name][1]))
        #logging.debug('registered correctly')
        if len(args) > 1:
            puburl = args.pop(0)
            suburl = args.pop(0)
            if puburl and suburl:
                self.connect_pubsub_north(puburl, suburl)
        else:
            logging.warning('No PUB/SUB interface configured')

    def connect_pubsub_north(self, puburl, suburl):
        """
         Connect north PUB/SUB sockets
        :param puburl:
        :param suburl:
        :return:
        """
        puburls = puburl.decode('utf8').split(',')
        suburls = suburl.decode('utf8').split(',')

        #logging.debug("got puburls", puburls, " and suburls ", suburls)
        for deal in self.dealerurl.split(','):
            if 'tcp' in deal:
                dealerport = deal.split(':')[-1]
                for url in puburls:
                    if 'tcp' in url:
                        port = url.split(':')[-1]
                        pubport = int(port)
                        self.sub_northurl = self.dealerurl.replace(dealerport, "%d" % pubport)
                for url in suburls:
                    if 'tcp' in url:
                        port = url.split(':')[-1]
                        subport = int(port)
                        self.pub_northurl = self.dealerurl.replace(dealerport, "%d" % subport)

            elif 'ipc' in deal:
                #logging.debug("IPC: ", deal.split('/')[-1])
                return
            else:
                logging.warning("dealerurl is not tcp or ipc, pub/sub not supported!")
                return

        #logging.debug("Found pubNurl %s and subNurl %s, connecting.." % (self.pub_northurl, self.sub_northurl))
        self.pub_north_sock.connect(self.pub_northurl)
        self.sub_north_sock.connect(self.sub_northurl)
        self.sub_north_stream = zmq.eventloop.zmqstream.ZMQStream(self.sub_north_sock, self.IOLoop)
        self.sub_north_stream.on_recv(self.on_sub_north_msg)
        self.pub_north_stream = zmq.eventloop.zmqstream.ZMQStream(self.pub_north_sock, self.IOLoop)
        self.pub_north_stream.on_recv(self.on_pub_north_msg)

        self.pub_southub = True

    def dest_invalid(self, args):
        """
        Function called if the command received is NODST
        This message travels through the bus to notify a client that it tied
        to reach an unregistered client.
        The current implementation is rather confusing as src is the client
        that emitted the initial message and is then the destination of the
        NODST message.
        :warning: This should be changed for clarity.
        :param args: Frames following the command in the original message
        :return: None
        """
        src = args.pop(0)
        dst = args.pop(0)
        if src in self.local_cli:
            self.router.send_multipart([src,
                                        DD.bPROTO_VERSION,
                                        DD.bCMD_NODST,
                                        dst])
        elif src in self.dist_cli:
            self.router.send_multipart([self.dist_cli[src][0],
                                        DD.bPROTO_VERSION,
                                        DD.bCMD_NODST,
                                        src,
                                        dst])
        else:
            logging.warning('problem with NODST')

    @abc.abstractmethod
    def forwardpt(self, source='', args=None):
        # implemented in sub classes
        """
        Forward plaintext message
        :param source:
        :param args:
        """
        pass

    @abc.abstractmethod
    def forward(self, source='', args=None):
        # implemented in sub classes
        """
        Forward message
        :param source:
        :param args:
        """
        pass

    @staticmethod
    def dummy(args):
        """

        :param args:
        """
        pass

    def print_topics(self):
        """
        List subscripbed topics

        """
        print("North topics")
        pprint(self.topic_north)
        print("South topics")
        pprint(self.topic_south)

    def debug_topics(self):
        """


        """
        #logging.debug("North topics: %s" % str(self.topic_north))
        #logging.debug("South topics: %s" % str(self.topic_south))

    def remove_sub_north(self, topic):
        """
        Remove topic from north subscription list
        :param topic:
        """
        if topic in self.topic_north:
            subs = self.topic_north[topic]
            subs -= 1
            if subs == 0:
                del self.topic_north[topic]
            else:
                self.topic_north[topic] = subs
        else:
            logging.warning("Unsubscribe from unknown topic: %s" % topic)
        self.debug_topics()

    def add_sub_north(self, topic):
        """
         Add topic to north subscription list
        :param topic:
        """
        if topic in self.topic_north:
            subs = self.topic_north[topic]
            subs += 1
            self.topic_north[topic] = subs
        else:
            self.topic_north[topic] = 1
        self.debug_topics()

    def remove_sub_south(self, topic):
        """
        remove topic from south subscription list
        :param topic:
        """
        if topic in self.topic_south:
            subs = self.topic_south[topic]
            subs -= 1
            if subs == 0:
                del self.topic_south[topic]
            else:
                self.topic_south[topic] = subs
        else:
            logging.warning("Unsubscribe from unknown topic: %s" % topic)
        self.debug_topics()

    def add_sub_south(self, topic):
        """
         Add topic to south subcription list
        :param topic:
        """
        if topic in self.topic_south:
            subs = self.topic_south[topic]
            subs += 1
            self.topic_south[topic] = subs
        else:
            self.topic_south[topic] = 1
        self.debug_topics()

    # path vector routing :-)
    def add_me(self, msg):
        # I'm the first node
        """
        Add this broker to PUB/SUB routing vector
        :param msg:
        :return:
        """
        if len(msg) >= 3:
            msg[1] = self.rname + msg[1]

        return msg

    def already_passed(self, msg):
        """
        Check if PUB/SUB message has passed this broker before
        :param msg:
        :return:
        """
        if len(msg) == 2:
            return False
        if len(msg) >= 3:
            if self.rname in msg[1]:
                return True
        return False

    def on_sub_north_msg(self, msg):
        """
        Function called automatically when a message is received on the
        socket connected to the broker.
        :param msg: Message received from the broker
        :return: None
        """

        #logging.debug("on_sub_north_msg, got: %s" % str(msg))
        # Messages from north should be published south
        if self.already_passed(msg):
            return

        msg = self.add_me(msg)
        # send to the south brokers
        self.pub_south_sock.send_multipart(msg)

        # send to the south clients
        topic = msg[0].decode()
        message = msg[2]
        source = msg[3]

        prefix = topic
        topic = topic.split('.')[1]
        try:
            client_to_send = set()
            for sons_topic in self.topics_trie.prefixes(prefix):
                sons_topic = sons_topic[0]
                for clientid in self.topics_trie[sons_topic]:
                    client_to_send.add(clientid)
            for client in client_to_send:
                self.router.send_multipart(
                    [client, DD.bPROTO_VERSION, DD.bCMD_PUB, source, topic.encode(), message])
        except BaseException as e:
            logging.error("Exception in on_sub_north_msg: " % str(e))
            pass

    #
    # Definition of the behavior in case of message received from the broker
    #

    def on_pub_north_msg(self, msg):
        """
        Function called automatically when a message is received on the
        socket connected to the router.
        :param msg: Message received from the router
        :return: None
        """
        #logging.debug("on_pub_north_msg, got: %s" % str(msg))
        a = msg[0]
        cmd = a[0]
        body = a[1:]

        if cmd == 1:
            logging.info(" + Got subscription for: %s" % str(body))
            self.add_sub_north(body)
        if cmd == 0:
            logging.info(" - Got unsubscription for: %s" % str(body))
            self.remove_sub_north(body)
        # subscriptions from north should continue down
        self.sub_south_sock.send_multipart(msg)

    def on_sub_south_msg(self, msg):
        """
        Function called automatically when a message is received on the
        socket connected to the broker.
        :param msg: Message received from the broker
        :return: None
        """

        # messages received from south should be published south and north
        #logging.debug("on_sub_south_msg, got: '%s'" % str(msg))
        if not self.already_passed(msg):
            msg = self.add_me(msg)

            # send to the north brokers
            self.pub_north_sock.send_multipart(msg)
            # send to the south brokers
            self.pub_south_sock.send_multipart(msg)

            # send to the south clients
            topic = msg[0].decode()
            message = msg[2]
            source = msg[3]

            prefix = topic
            topic = topic.split('.')[1]
            try:
                client_to_send = set()
                for sons_topic in self.topics_trie.prefixes(prefix):
                    sons_topic = sons_topic[0]
                    for clientid in self.topics_trie[sons_topic]:
                        client_to_send.add(clientid)
                for client in client_to_send:
                    self.router.send_multipart(
                        [client, DD.bPROTO_VERSION, DD.bCMD_PUB, source, topic.encode(), message])
            except BaseException as e:
                logging.error("Exception in on_sub_south_msg: %s" % str(e))
                pass

    def on_pub_south_msg(self, msg):
        """dealer seems
        Function called automatically when a message is received on the
        socket connected to the broker.
        :param msg: Message received from the broker
        :return: None
        """
        #logging.debug("on_pub_south_msg, got: %s", str(msg))
        # subscriptions from south should be subscribed north & south
        a = msg[0]
        cmd = a[0]
        body = a[1:]

        if cmd == 1:
            logging.info(" + Got subscription for: %s" % str(body))
            self.add_sub_south(body)
        if cmd == 0:
            logging.info(" - Got unsubscription for: %s" % str(body))
            self.remove_sub_south(body)

        self.sub_north_sock.send_multipart(msg)
        self.sub_south_sock.send_multipart(msg)

    @abc.abstractmethod
    def pub(self, source, msg):
        """
        Function called when a PUB command message is received on the
        socket connected to the router.
        :param source: Identity of the sender
        :param msg: Message received from the router
        :return: None
        """
        # implemented in sub-classes
        pass

    # @abc.abstractmethod
    # def pub_public(self, source, msg):
    #    # implemented in sub-classe
    #    pass

    @abc.abstractmethod
    def sub(self, source, msg):
        """
        Function called when a SUB command message is received on the
        socket connected to the router.
        :param source: Identity of the sender
        :param msg: topic to subscribe
        :return: None
        """
        # implemented in sub-classes
        pass

    def unsub(self, source, msg):
        """
        Function called when a UNSUB command message is received on the
        socket connected to the router.
        :param source: Identity of the sender
        :param msg: topic to unsubscribe
        :return: None
        """

        R = msg.pop(0)
        topic = msg.pop(0)
        scopestr = msg.pop(0).decode()

        try:
            if source + R in self.registered_client:
                r = self.local_cli[source][1]
        except KeyError:
            logging.warning("Got unsub from unknown source %s" % str(source))
            return

        #print("START subscrioptions: ")
        #pprint(self.subscriptions)
        #print("START topic_south:")
        #pprint(self.topic_south)
        #print("START topics_trie:")
        #pprint(self.topics_trie)

        topic = '.'.join([r, topic.decode()])
        logging.info(" - Got unsubscription for: %s from %s" % (str(topic),source))


       # print("scopestr: ", scopestr)
        if scopestr != "noscope":
                scopearray = scopestr.strip().split('/')
                # delete the empty strings ''
                length = len(scopearray)
                scopearray = scopearray[1:length - 1]
                i = 0
                j = 0
                for scope in scopearray:
                    if scope == "*":
                        scopearray[j] = self.scopearray[i]
                        topic = '/'.join([topic, scopearray[j]])
                        i += 1
                    elif scope is '':
                        # TODO: replace DATAPT with normal sendmsg() call
                        self.router.send_multipart([source, DD.bPROTO_VERSION, DD.bCMD_DATAPT, self.name,
                                                    b'ERROR: the scope should be like "1/2/3"'])
                        return
                    else:
                        topic = '/'.join([topic, scopearray[j]])
                        i += 1
                    j += 1
                topic += '/'
        topic = topic.encode()

        #print("topic after  scope added: ", topic)
        if source in self.subscriptions:
            if topic in self.subscriptions[source]:
                self.subscriptions[source].remove(topic)

                if topic in self.topic_south:
                    if self.topic_south[topic] == 1:
                        self.topics_trie.pop(topic.decode())
                    else:
                        if topic.decode() in self.topics_trie:
                            self.topics_trie[topic.decode()].remove(source)
                self.remove_sub_south(topic)
        # on_pub_msg should receive a message like : [b'CmdTopic'], where Cmd=\x00 for an unsub
        msg = b'\x00' + topic
        self.sub_north_sock.send_multipart([msg])
        self.sub_south_sock.send_multipart([msg])

#        print("END subscrioptions: ")
#        pprint(self.subscriptions)
#        print("END topic_south:")
#        pprint(self.topic_south)
#        print("END topics_trie:")
#        pprint(self.topics_trie)

    #
    # Definition of the behavior in case of message received from the broker
    #
    @abc.abstractmethod
    def on_router_msg(self, msg):
        """
        Function called automatically when a message is received on the
        socket connected to the router.
        :param msg: Message received from the router
        :return: None
        """
        # implemented in sub-classes
        pass

    @abc.abstractmethod
    def reg_local_cli(self, name, data):
        """
        Function called if the command received is CHALLOK.
        After the client done the challenge, this function check the result
        :param name: Identity of the client
        :param data: contained the decrypted number sent by the client and the customer name of the client
        :return: None
        """
        # implemented in sub-classes
        pass

    @abc.abstractmethod
    def add_local_cli(self, identity, args):
        """
        Function called if the command received is ADDLCL.
        This function adds a local client to the broker.
        :param identity: Identity of the client
        :param args: Frames following the command in the original message
        :return: None
        """
        # implemented in sub-classes
        pass


    def add_dist_cli(self, source, args):
        """
        Function called if the command received is ADDDCL.
        This function adds a distant client earlier added to the broker
        identified by the argument source.
        :param source: Identity of the broker which send the message
        :param args: Frames following the command in the original message
        :return: None
        """
        if len(args) < 2:
            return
        name = args.pop(0)
        dist = args.pop(0)
        if name not in self.dist_cli:
            self.dist_cli[name] = [source, dist]
            logging.info(' + Added distant client: %s (%s)' % (name, dist))

        if self.state == DD.S_REGISTERED:
            self.add_client_up(name, int(dist))

    def add_local_br(self, source, args):
        """
        Function called if the command received is ADDBR
        Adds a broker to the brokers list
        :param source: Identity of the broker which sent the request
        :param args: Frames following the command in the original message
        """

        # TODO: should do authentication here too!

        if source not in self.local_br:
            logging.info(' + Added broker: %s' % str(source))
            self.local_br[source] = [0]

        self.router.send_multipart([source, DD.bPROTO_VERSION, DD.bCMD_REGOK,
                                    self.pub_southurl.encode('utf8'), self.sub_southurl.encode('utf8')])

    @abc.abstractmethod
    def unreg_cli(self, identity, args):
        """
        Function called if the command received is UNREG
        Deletes a client previously registered locally.
        :param identity: name of the client to delete
        :param args: Frames following the command in the original message
        """
        # implemented in sub classes
        pass

    def unreg_dist_cli(self, broker, args):
        """
        Function called if the command received is UNREGDCLI
        Deletes a client previously registered as distant. A verification
        is performed so that only a broker above a client can require
        its unregistration.
        :param broker: Broker which sent the message
        :param args: Frames following the command in the original message
        :return:
        """
        if len(args) < 1:
            return
        name = args.pop(0)
        if name in self.dist_cli and self.dist_cli[name][0] == broker:
            logging.info(' - Deleting distant client: %s' % str(name))
            del self.dist_cli[name]
            self.del_cli_up(name)

    def unreg_br(self, name, args):
        """
        Unregister a a local broker, first unregisteres reported clients then broker
        :param name: Broker name (bytes)
        :param args:
        """
        tmp_to_del = []
        logging.info(' - Unregistering broker: %s' % str(name))
        for cli in self.dist_cli:
            if self.dist_cli[cli][0] == name:
                tmp_to_del.append(cli)

        for i in tmp_to_del:
            self.unreg_dist_cli(name, [i])
        self.local_br.pop(name)

    # This doesn't work correctly currently, treats unregistered as registered.
    # Bad in case of broker crash and restart
    def pong(self, identity, args):
        """
        Answers a PING
        :param identity: socket identity to answer on
        :param args:
        """
        if identity in self.local_cli:
            self.router.send_multipart([identity, DD.bPROTO_VERSION, DD.bCMD_PONG])
        elif identity in self.local_br:
            self.router.send_multipart([identity, DD.bPROTO_VERSION, DD.bCMD_PONG])
        else:
            logging.warning("Ping from unregistered client/broker: %s" % str(identity))

    # plain text message from local to another client
    @abc.abstractmethod
    def sendpt(self, source, args):
        # implemented in sub classes
        """
        Send plaintext raw message
        :param source:
        :param args:
        """
        pass

    # encrypted message from local to another client
    @abc.abstractmethod
    def send(self, source, args):
        # implemented in sub classes
        """
        Send raw message
        :param source:
        :param args:
        """
        pass

    # message from local client to a public client
    # @abc.abstractmethod
    # def send_public(self, source, args):
    #    # implemented in sub classes
    #    pass

    #
    # Callbacks
    #
    def ask_registration(self):
        """
        Try to register

        """
        logging.info('Trying to register')
        # before trying to register, close DEALER socket, open it again, then send message
        # no linger period, pending messages shall be discarded immediately when the socket is closed
        self.dealer.setsockopt(zmq.LINGER, 0)
        self.dealer_stream.close()
        self.dealer.close()
        self.dealer = self.ctx.socket(zmq.DEALER)
        self.dealer.setsockopt(zmq.IDENTITY, self.name)
        self.dealer.connect(self.dealerurl)
        self.dealer_stream = zmq.eventloop.zmqstream.ZMQStream(self.dealer, self.IOLoop)
        self.dealer_stream.on_recv(self.on_dealer_msg)
        self.dealer.send_multipart([DD.bPROTO_VERSION, DD.bCMD_ADDBR, self.name])

    @abc.abstractmethod
    def check_cli_timeout(self):
        # implemented in sub classes
        """
        Check if clients have timed out

         """
        pass

    def ping(self):
        """
        Send ping and check reply timeout

        """
        self.timeout += 1
        if self.timeout > 3:
            logging.warning('Dealer seems dead')
            self.state = DD.S_ROOT
            self.heartbeat.stop()
            self.register_loop.start()
        else:
            self.dealer.send_multipart([DD.bPROTO_VERSION, DD.bCMD_PING])

    def check_br_timeout(self):
        """
        Check if brokers have timed out

        """
        tmp_to_del = []
        for br in self.local_br:
            if self.local_br[br][0] < 3:
                self.local_br[br][0] += 1
            else:
                tmp_to_del.append(br)

        for br in tmp_to_del:
            self.unreg_br(br, [])

    #
    # Other functions
    #
    # add_cli_up, name should be in b'' format, not string
    def add_client_up(self, name, dist):
        """
        Add a client to broker above
        :param name:
        :param dist:
        :return:
        """
        if self.state == DD.S_ROOT:
            return
        # print("add_cli_up -> Sending ADDDCL dist: ",str(dist + 1).encode('utf8'), " ", name)
        self.dealer.send_multipart([DD.bPROTO_VERSION, DD.bCMD_ADDDCL, name, str(dist + 1).encode('utf8')])

    #
    # forward in plain text
    #
    @abc.abstractmethod
    def forward_locallypt(self, src, src_name, dst, msg):
        # implemented in sub classes
        """
         Forward plaintext message locally
         :param src:
         :param src_name:
         :param dst:
         :param msg:
         """
        pass

    @abc.abstractmethod
    def forward_downpt(self, dst_name, source_name, msg):
        # implemented in sub classes
        """
         Forward plaintext message downwarsd
         :param dst_name:
         :param source_name:
         :param msg:
         """
        pass

    @abc.abstractmethod
    def forward_uppt(self, dst_name, source_name, msg):
        # implemented in sub classes
        """
         Forward plaintext message upwards
         :param dst_name:
         :param source_name:
         :param msg:
         """
        pass

    #
    # forward encrypted message
    #
    @abc.abstractmethod
    def forward_locally(self, src, src_name, dst, msg):
        # implemented in sub classes
        """
         Forward message to local client
         :param src:
         :param src_name:
         :param dst:
         :param msg:
         """
        pass

    @abc.abstractmethod
    def forward_down(self, dst_name, source_name, msg):
        # implemented in sub classes
        """
         Forward message downwards
         :param dst_name:
         :param source_name:
         :param msg:
         """
        pass

    @abc.abstractmethod
    def forward_up(self, dst_name, source_name, msg):
        # implemented in sub classes
        """
        Forward message upwards
        :param dst_name:
        :param source_name:
        :param msg:
        """
        pass

    def del_cli_up(self, name):
        """

        :param name:
        """
        if self.state == DD.S_REGISTERED:
            self.dealer.send_multipart([DD.bPROTO_VERSION,
                                        DD.bCMD_UNREGDCLI,
                                        name])

    #
    # Methods related to the command line inputs
    #

    @abc.abstractmethod
    def on_stdin(self, dummy, other_dummy):
        # implemented in sub classes
        """

         :param dummy:
         :param other_dummy:
         """
        pass

    @staticmethod
    def cli_usage():
        """
        CLI commands
        """
        print('Commands :')
        print('reg - prints registered clients and brokers')
        print('topics - print registered topics')
        print('exit - shut down the broker')

    def shutdown(self):
        """
        Shutdown the broker
        """
        logging.info("Shutting down broker")
        self.heartbeat.stop()
        self.register_loop.stop()
        self.cli_timeout_loop.stop()
        self.br_timeout_loop.stop()
        self.IOLoop.stop()


class BrokerSafe(Broker):
    """

    :param name:
    :param routerurl:
    :param dealerurl:
    :param scope:
    :param pubsub:
    """

    def __init__(self, name, routerurl, dealerurl, scope, keys='broker-keys.json'):
        super().__init__(name, routerurl, dealerurl, scope)
        self.hashes = None
        self.privkey = None
        self.pubkey = None
        self.keys = keys
        try:
            f = open(self.keys)
        except:
            logging.critical("Could not open keyfile: %s" % self.keys)
            sys.exit(1)

        self.hashes = json.load(f)
        self.privkey = nacl.public.PrivateKey(self.hashes['dd']['privkey'], encoder=nacl.encoding.Base64Encoder)
        self.pubkey = nacl.public.PublicKey(self.hashes['dd']['pubkey'], encoder=nacl.encoding.Base64Encoder)
        for n in self.hashes:
            if 'r' in self.hashes[n]:
                self.tenants.append(self.hashes[n]['r']+".")
        for cust in self.hashes.keys():
            cookie_string = self.hashes[cust]['R']
            self.hashes[cust]['R'] = int(cookie_string).to_bytes(8,byteorder=sys.byteorder)

    def add_local_cli(self, identity, args):
        """
        Add local client
        :param identity:
        :param args:
        """
        #print("add_local_cli called")
        if identity not in self.local_cli:
            hashval = args.pop(0).decode()
            if hashval != 'unsafe':
                cust_found = False
                for cust in self.hashes.keys():
                    if cust == hashval:
                        client_pubkey = nacl.public.PublicKey(self.hashes[cust]['pubkey'],
                                                              encoder=nacl.encoding.Base64Encoder)
                        box = nacl.public.Box(self.privkey, client_pubkey)
                        nonce = nacl.utils.random(nacl.public.Box.NONCE_SIZE)
                        # Then, we encrypt the customer random number

                        encryptednumber = box.encrypt(self.hashes[cust]['R'], nonce)
#                        import binascii
#                        print("plaintext len",len(str(self.hashes[cust]['R']).encode())," ",binascii.hexlify(str(self.hashes[cust]['R']).encode()))
#                        print("encryptednumber len", len(encryptednumber), " ",binascii.hexlify(encryptednumber))
#                        print("nounce: ", len(encryptednumber.nonce)," ", binascii.hexlify(encryptednumber.nonce))
#                        print("ciphertext: ", len(encryptednumber.ciphertext), "  ", binascii.hexlify(encryptednumber.ciphertext))

                        # And we send the encrypted number to the client
#                        print("Send challenge to client ", identity)
                        self.router.send_multipart([identity, DD.bPROTO_VERSION, DD.bCMD_CHALL, encryptednumber])
                        cust_found = True
                        break
                if not cust_found:
                    # handle error if the customer is not found
                    logging.warning("Authentication failed")
                    msg = "Authentication failed"
                    self.router.send_multipart(
                        [identity, DD.bPROTO_VERSION, DD.bCMD_DATAPT, self.name, msg.encode()])
            else:
                error_msg = "An unsafe client can't be connected to a safe broker."
                self.router.send_multipart([identity, DD.bPROTO_VERSION, DD.bCMD_DATA, self.name, error_msg.encode()])

    def reg_local_cli(self, identity, data):
        """
        Function called if the command received is CHALLOK.
        After the client done the challenge, this function check the result
        :param identity: Identity of the client
        :param data: contained the decrypted number sent by the client and the customer name of the client
        :return: None
        """
        if identity not in self.local_cli:
            # Check if the sent decrypted number match with the random number
            decryptednumber = data.pop(0)
            keyhash = data.pop(0).decode()
            clientname = data.pop(0)  # e.g.: client1
            if clientname == b'public':
                self.router.send_multipart(
                    [identity, DD.bPROTO_VERSION, DD.bCMD_DATAPT, self.name, b"ERROR: protected name"])
                return
            if decryptednumber == self.hashes[keyhash]['R']:
                customer_r = self.hashes[keyhash]['r']  # e.g.: A
                self.local_cli[identity] = (clientname, customer_r)
                self.local_cli_timeout[identity] = 0
                prefix_clientname = '.'.join([customer_r, clientname.decode()])  # e.g.: A.client1
                prefix_clientname = prefix_clientname.encode('utf8')
                self.reverse_local_cli[prefix_clientname] = [identity, 0]
                self.registered_client.add(identity + decryptednumber)
                self.router.send_multipart([identity, DD.bPROTO_VERSION, DD.bCMD_REGOK, decryptednumber])
                logging.info(' + Added local client :%s' % str(prefix_clientname))
                if self.state == DD.S_REGISTERED:
                    self.add_client_up(prefix_clientname, 0)
            else:
                logging.warning("%s:  : Authentication failed" % str(clientname))
                msg = "Authentication failed"
                self.router.send_multipart(
                    [identity, DD.bPROTO_VERSION, DD.bCMD_DATAPT, self.name, msg.encode()])


    # special behaviour required
    # source is not public but topic.startswithc("public.") -> do not add "tenant." to topic
    def pub(self, source, msg):
        """
        Publish message
        :param source:
        :param msg:
        """
        srcpublic = False
        dstpublic = False
        print("pub called with s: ", source, "and msg:", msg)
        cookie = msg.pop(0)
        topic = msg[0].decode()

        if source + cookie not in self.registered_client:
            logging.warning("Unregisterd source %s trying to publish"%(source+cookie))
            return

        name, tenant = self.local_cli[source]
        if tenant == "public":
            srcpublic = True

        if topic.startswith("public."):
            dstpublic = True

        topic_simple = topic  # without the scope
        if topic[-1] != '$':
            topic = topic + '/' + self.scope + '/'
        else:
            topic = topic[0:-1]

        # if destination is public, topic remains the same but source gets the tenant prefix
        if dstpublic:
            prefix_topic = topic
            name = '.'.join([tenant, name.decode()]).encode()
        else:
            # if destination is not public, keep name but add tenant to topic
            prefix_topic = '.'.join([tenant, topic])


        msg[0] = prefix_topic.encode()
        #logging.debug("Got a pub: %s from: %s" % (str(msg), str(name)))
        msg = msg + [name]
        if not self.already_passed(msg):
            self.add_me(msg)
            # send to the north brokers
            self.pub_north_sock.send_multipart(msg)
            # send to the south brokers
            self.pub_south_sock.send_multipart(msg)
            # send to the south clients
            print("message before clients: ", msg)
            message = msg[2]
            try:
                client_to_send = set()
                for sons_topic in self.topics_trie.prefixes(prefix_topic):
                    sons_topic = sons_topic[0]
                    for clientid in self.topics_trie[sons_topic]:
                        client_to_send.add(clientid)
                for client in client_to_send:
                    self.router.send_multipart(
                        [client, DD.bPROTO_VERSION, DD.bCMD_PUB, name, topic_simple.encode(), message])
            except BaseException as e:
                logging.error("Exception in pub: %s" % str(e))
                pass


    # Subscribe, no special checks needed for public/customer tenants
    # same behaviour in all cases (no tenant can subscribe to others topics)
    def sub(self, source, msg):
        """
         Subscribe to topic
        :param source:
        :param msg:
        :return:
        """
        cookie = msg.pop(0)
        topic = msg.pop(0).decode()
        topic_orig = topic
        scopestr = msg.pop(0).decode()
        # check that client is allowed to publish
        if source + cookie not in self.registered_client:
            logging.warning("unregistered client %s trying to subscribe!"%(source+cookie))
            return

        tenant = self.local_cli[source][1]

        if topic == 'public':
            self.router.send_multipart([source, DD.bPROTO_VERSION, DD.bCMD_DATAPT, self.name, b"ERROR: protected name"])
            return
            # TODO: replace DATAPT with normal sendmsg() call

        if scopestr != "noscope":
            scopearray = scopestr.strip().split('/')
            # delete the empty strings ''
            length = len(scopearray)
            scopearray = scopearray[1:length - 1]
            i = 0
            j = 0
            for scope in scopearray:
                if scope == "*":
                    scopearray[j] = self.scopearray[i]
                    topic = '/'.join([topic, scopearray[j]])
                    i += 1
                elif scope is '':
                    # TODO: replace DATAPT with normal sendmsg() call
                    self.router.send_multipart([source, DD.bPROTO_VERSION, DD.bCMD_DATAPT, self.name,
                                                b'ERROR: the scope should be like "1/2/3"'])
                    return
                else:
                    topic = '/'.join([topic, scopearray[j]])
                    i += 1
                j += 1
            topic += '/'
        # reply with SUBOK
        self.router.send_multipart([source, DD.bPROTO_VERSION, DD.bCMD_SUBOK, topic_orig.encode(),scopestr.encode()])

#            print("START subscrioptions: ")
#            pprint(self.subscriptions)
#            print("START topic_south:")
#            pprint(self.topic_south)
#            print("START topics_trie:")
#            pprint(self.topics_trie)

        # Register subscription locally and submit to north and south brokers
        topic = '.'.join([tenant, topic])
        topic = topic.encode()
        new = 0
        if source in self.subscriptions:
            if topic not in self.subscriptions[source]:
                self.subscriptions[source].append(topic)
                new += 1
        else:
            self.subscriptions[source] = [topic]
            new +=1

        if topic.decode() in self.topics_trie:
            if source not in self.topics_trie[topic.decode()]:
                self.topics_trie[topic.decode()].append(source)
                new += 1
        else:
            self.topics_trie[topic.decode()] = [source]
            new += 1

        # On_pub_msg should receive a message like : [b'CmdTopic'], where Cmd=\x01 for a sub
        # only do this if it was actually a new sub
        if new == 2:
            self.add_sub_south(topic)
            msg = b'\x01' + topic
            self.sub_north_sock.send_multipart([msg])
            self.sub_south_sock.send_multipart([msg])

        logging.info(" + Added subscription for: %s" % str(topic))

#            print("new: ", new)
#            print("END subscrioptions: ")
#            pprint(self.subscriptions)
#            print("END topic_south:")
#            pprint(self.topic_south)
#            print("END topics_trie:")
#            pprint(self.topics_trie)


    def on_router_msg(self, msg):
        #print("on router msg ", msg)
        """
        Handle messages from south
        :param msg:
        :return:
        """
        source = msg.pop(0)
        tmp = msg.pop(0)
        if tmp != DD.bPROTO_VERSION:
            logging.warning('Different protocols in use, message from: %s' % str(source))
            return

        cmd = msg.pop(0)
     #   print("Got command:", cmd)
        if cmd != DD.bCMD_ADDLCL:
            if source in self.local_cli_timeout:
                 self.local_cli_timeout[source] = 0
            elif source in self.local_br:
                self.local_br[source][0] = 0

        if cmd in self.router_cmds:
            f = self.router_cmds[cmd]
            f(source, msg)
        else:
            logging.warning('Ununderstood command from %s: %d'
                            % (str(source), int.from_bytes(cmd, 'little')))

    def unreg_cli(self, identity, args):
        """
         Unregister client
        :param identity:
        :param args:
        """
        if identity in self.local_cli:
            cust = args[0].decode()
            name = args[1].decode()
            prefix_name = '.'.join([cust, name]).encode()
            logging.info(' - Deleting local client %s' % str(prefix_name))
            if identity in self.local_cli_timeout:
                del self.local_cli_timeout[identity]
            if identity in self.local_cli:
                del self.local_cli[identity]
            if prefix_name in self.reverse_local_cli:
                del self.reverse_local_cli[prefix_name]
            try:
                R = args[2]
                self.registered_client.remove(identity + R)
            except:
                pass
            self.del_cli_up(prefix_name)

            # delete the subscriptions of this client
            if name in self.subscriptions:
                del self.subscriptions[identity]
        else:
            logging.warning('Requested to delete an unknown client')

    def forwardpt(self, source='', args=None):
        """
        Forward plaintext message
        :param source:
        :param args:
        :return:
        """
        if not args:
            args = []
        if len(args) < 2:
            return
        src_name = args.pop(0)
        dst_name = args.pop(0)
        # if the dest is a local client
        if dst_name in self.reverse_local_cli:
            self.forward_locallypt(source, src_name, self.reverse_local_cli[dst_name][0], args)
        # if the dest is a distant client
        elif dst_name in self.dist_cli:
            self.forward_downpt(dst_name, src_name, args)
        elif self.state == DD.S_ROOT:
            self.dest_invalid([source, dst_name, args])
        else:
            self.forward_uppt(dst_name, src_name, args)

    def forward(self, source='', args=None):
        """
        Forward message
        :param source:
        :param args:
        :return:
        """
        if not args:
            args = []
        if len(args) < 2:
            return
        src_name = args.pop(0)
        dst_name = args.pop(0)
        # if the dest is a local client
        if dst_name in self.reverse_local_cli:
            self.forward_locally(source, src_name, self.reverse_local_cli[dst_name][0], args)
        # if the dest is a distant client
        elif dst_name in self.dist_cli:
            self.forward_down(dst_name, src_name, args)
        elif self.state == DD.S_ROOT:
            self.dest_invalid([source, dst_name, args])
        else:
            self.forward_up(dst_name, src_name, args)

    def sendpt(self, source, args):
        """
         Plain text message from local to another client
        :param source:
        :param args:
        :return:
        """
        if len(args) < 1:
            return
        R = args.pop(0)
        # check if the client is registered
        if source + R in self.registered_client:
            dst_name = args.pop(0).decode()
            (source_name, customer_r) = self.local_cli[source]
            prefix_dst_name = '.'.join([customer_r, dst_name]).encode()
            # if the dest is a local client
            if prefix_dst_name in self.reverse_local_cli:
                self.forward_locallypt(source, source_name, self.reverse_local_cli[prefix_dst_name][0], args)
            # if the dest is a distant client
            elif prefix_dst_name in self.dist_cli:
                self.forward_downpt(prefix_dst_name, source_name, args)
            elif self.state == DD.S_ROOT:
                self.dest_invalid([source, dst_name.encode(), args])
            else:
                self.forward_uppt(prefix_dst_name, source_name, args)

    def send(self, source, args):
        """
         encrypted message from local to another client
        :param source:
        :param args:
        :return:
        """
        if len(args) < 1:
            return

        srcpublic = False
        dstpublic = False
        cookie = args.pop(0)
        dst_name = args.pop(0).decode()
        # check if the client is registered
        if source + cookie not in self.registered_client:
            logging.warning("Unregistered client %s trynig to send"%(source+cookie))
            return
        # check if source / destination is public
        source_name, tenant = self.local_cli[source]
        if tenant == "public":
            srcpublic = True
        if dst_name.startswith("public."):
            dstpublic = True

        # if destination is public, add tenant to source_name but not on prefix_dst_name
        if dstpublic and not srcpublic:
            source_name = '.'.join([tenant, source_name.decode()]).encode()
            prefix_dst_name = dst_name.encode()
        # if source is public and destination is public, don't add additional public. to prefix
        elif dstpublic and srcpublic:
            prefix_dst_name = dst_name.encode()
        # if source is public but not destination, check if public. should be added
        # if dst_name.startswith("tenant."), don't add public.
        elif srcpublic and not dstpublic:
            add_prefix = True
            if '.' in dst_name:
                for n in self.tenants:
                    if dst_name.startswith(n):
                        add_prefix = False
                        break

            if add_prefix:
                prefix_dst_name = '.'.join([tenant, dst_name]).encode()
            else:
                prefix_dst_name = dst_name.encode()
                source_name = '.'.join([tenant, source_name.decode()]).encode()
        else:
            prefix_dst_name = '.'.join([tenant, dst_name]).encode()


        # Forward the messages
        # if the dest is a local client
        if prefix_dst_name in self.reverse_local_cli:
            self.forward_locally(source, source_name, self.reverse_local_cli[prefix_dst_name][0], args)
        # if the dest is a distant client
        elif prefix_dst_name in self.dist_cli:
            self.forward_down(prefix_dst_name, source_name, args)
        # if I'm root broker but havent got client registered
        elif self.state == DD.S_ROOT:
            self.dest_invalid([source, dst_name.encode(), args])
        # else forward to higher level broker
        else:
            self.forward_up(prefix_dst_name, source_name, args)

    def check_cli_timeout(self):
        """
        Check client timeouts
        """
        tmp_to_del = []
        for client in self.local_cli_timeout:
            if self.local_cli_timeout[client] < 3:
                self.local_cli_timeout[client] += 1
            else:
                tmp_to_del.append(client)

        for client in tmp_to_del:
            name, cust = self.local_cli[client]
            self.unreg_cli(client, [cust.encode(), name])

    def forward_locallypt(self, src, src_name, dst, msg):
        """
        Forward locally in plain text
        :param src:
        :param src_name:
        :param dst:
        :param msg:
        """
        #logging.debug('Forwardpt "%s" locally to: %s' % (str(msg), str(self.local_cli[dst][0])))
        self.router.send_multipart([dst,
                                    DD.bPROTO_VERSION,
                                    DD.bCMD_DATAPT,
                                    src_name,
                                    msg.pop()])

    def forward_downpt(self, dst_name, source_name, msg):
        """
        Forward down plaintext
        :param dst_name:
        :param source_name:
        :param msg:
        """
        #logging.debug('Forwardpt "%s" to %s through %s' % (str(msg), str(dst_name), str(self.dist_cli[dst_name][0])))
        self.router.send_multipart([self.dist_cli[dst_name][0],
                                    DD.bPROTO_VERSION,
                                    DD.bCMD_FORWARDPT,
                                    source_name,
                                    dst_name,
                                    msg.pop()])

    def forward_uppt(self, dst_name, source_name, msg):
        """

        :param dst_name:
        :param source_name:
        :param msg:
        """
        #logging.debug('Unknown client %s, forwarding up' % dst_name)
        self.dealer.send_multipart([DD.bPROTO_VERSION,
                                    DD.bCMD_FORWARDPT,
                                    source_name,
                                    dst_name,
                                    msg.pop()])

    #
    # forward encrypted message
    #

    def forward_locally(self, src, src_name, dst, msg):
        """
        Forward message locally
        :param src:
        :param src_name:
        :param dst:
        :param msg:
        """
        #logging.debug('Forward "%s" locally to: %s' % (str(msg), str(self.local_cli[dst][0])))
        m = [dst,DD.bPROTO_VERSION,DD.bCMD_DATA,src_name,msg[0]]
        self.router.send_multipart(m)

    def forward_down(self, dst_name, source_name, msg):
        """
        Foward message down
        :param dst_name:
        :param source_name:
        :param msg:
        """
        #logging.debug('Forward "%s" to %s through %s' % (str(msg), str(dst_name), str(self.dist_cli[dst_name][0])))
        self.router.send_multipart([self.dist_cli[dst_name][0],
                                    DD.bPROTO_VERSION,
                                    DD.bCMD_FORWARD,
                                    source_name,
                                    dst_name,
                                    msg.pop()])

    def forward_up(self, dst_name, source_name, msg):
        """
         Foward message up
        :param dst_name:
        :param source_name:
        :param msg:
        """
        #logging.debug('Unknown client %s, forwarding up' % dst_name)
        self.dealer.send_multipart([DD.bPROTO_VERSION,
                                    DD.bCMD_FORWARD,
                                    source_name,
                                    dst_name,
                                    msg.pop()])

    def on_stdin(self, dummy, other_dummy):
        """
        Methods related to the command line inputs

        :param dummy:
        :param other_dummy:
        """
        cmd = sys.stdin.readline().split(maxsplit=2)
        if len(cmd) == 0:
            self.cli_usage()
        elif 'reg' == cmd[0]:
            print('List of local brokers :')
            for b in self.local_br:
                print(b)
            print('List of local clients :')
            for c in self.reverse_local_cli:
                print(c)
            print('List of distant clients :')
            for c in self.dist_cli:
                print(c)
        elif 'topics' == cmd[0]:
            self.print_topics()
        elif 'exit' == cmd[0]:
            self.shutdown()
        else:
            self.cli_usage()
        print('---------------')


class BrokerUnsafe(Broker):
    """

    :param name:
    :param routerurl:
    :param dealerurl:
    :param scope:
    :param pubsub:
    """

    def __init__(self, name, routerurl, dealerurl, scope):
        super().__init__(name, routerurl, dealerurl, scope)

    def forwardpt(self, source='', args=None):
        """

        :param source:
        :param args:
        """
        pass

    def sendpt(self, source, args):
        """

        :param source:
        :param args:
        """
        pass

    def forward_uppt(self, dst_name, source_name, msg):
        """

        :param dst_name:
        :param source_name:
        :param msg:
        """
        pass

    def forward_locallypt(self, src, src_name, dst, msg):
        """

        :param src:
        :param src_name:
        :param dst:
        :param msg:
        """
        pass

    def forward_downpt(self, dst_name, source_name, msg):
        """

        :param dst_name:
        :param source_name:
        :param msg:
        """
        pass

    def reg_local_cli(self, name, data):
        """

        :param name:
        :param data:
        """
        pass

    def add_local_cli(self, name, args):
        """
        Function called when a client tries to register with the broker
        :param name: name of the client
        :param args: next argument, should contain the name of the customer
        :return: None
        """
        if name in self.local_cli:
            logging.info('%s asked for registration but is already registered'
                         % name)
            return

        hashval = args.pop(0).decode()
        if hashval == 'unsafe':
            customer = args.pop(0).decode()
            short_name = args.pop(0).decode()
            self.local_cli[name] = [0, customer, short_name]
            self.router.send_multipart([name, DD.bPROTO_VERSION, DD.bCMD_REGOK])
            logging.info(' + Added local client : %s' % name)
            if self.state == DD.S_REGISTERED:
                self.add_client_up(name, 0)
        else:
            error_msg = "A safe client can't be connected to an unsafe broker."
            self.router.send_multipart([name, DD.bPROTO_VERSION, DD.bCMD_DATAPT, self.name, error_msg.encode()])

    def pub(self, source, msg):
        """
        Function called when a PUB command message is received on the
        socket connected to the router.
        :param source: Identity of the sender
        :param msg: Message received from the router
        :return: None
        """
        topic = msg[0].decode()
        topic_simple = topic  # without the scope
        is_public = int.from_bytes(msg.pop(0), byteorder='little')

        if topic[-1] != '$':
            topic = topic + '/' + self.scope + '/'
        else:
            topic = topic[0:-1]

        customer = self.local_cli[source][1]
        if is_public == 1:
            prefix_topic = '.'.join(['public', topic])
            prefix_topic = prefix_topic.encode()
        else:
            prefix_topic = '.'.join([customer, topic])
            prefix_topic = prefix_topic.encode()
        msg[0] = prefix_topic

        #logging.debug("Got a pub: %s from %s" % (str(msg), str(source)))

        source_name = self.local_cli[source][2].encode()
        msg = msg + [source_name]

        if not self.already_passed(msg):
            self.add_me(msg)
            print("pub_north: ", msg)
            self.pub_north_sock.send_multipart(msg)
            print("pub_south: ", msg)
            self.pub_south_sock.send_multipart(msg)

            message = msg[2]
            try:
                client_to_send = set()
                for sons_topic in self.topics_trie.prefixes(prefix_topic):
                    sons_topic = sons_topic[0]
                    for clientid in self.topics_trie[sons_topic]:
                        client_to_send.add(clientid)
                for client in client_to_send:
                    print("Publishing message to:", [client, DD.bPROTO_VERSION, DD.bCMD_PUB, source_name, topic_simple.encode(), message])
                    self.router.send_multipart(
                        [client, DD.bPROTO_VERSION, DD.bCMD_PUB, source_name, topic_simple.encode(), message])
            except BaseException as e:
                logging.critical("Exception in on_subS_msg: %s" % str(e))
                pass

    def sub(self, source, msg):
        """

        :param source:
        :param msg:
        :return:
        """
        customer = self.local_cli[source][1]
        topic = msg.pop(0).decode()
        topic_orig = topic
        scopestr = msg.pop(0).decode()
        if scopestr != "noscope":
            scopearray = scopestr.strip().split('/')
            # delete the empty strings ''
            length = len(scopearray)
            scopearray = scopearray[1:length - 1]
            i = 0
            j = 0
            for scope in scopearray:
                if scope == "*":
                    scopearray[j] = self.scopearray[i]
                    topic = '/'.join([topic, scopearray[j]])
                    i += 1
                elif scope is '':
                    self.router.send_multipart(
                        [source, DD.bPROTO_VERSION, DD.bCMD_DATA, self.name, b'ERROR: the scope should be like 1/2/3'])
                    return
                else:
                    topic = '/'.join([topic, scopearray[j]])
                    i += 1
                j += 1
            topic += '/'

        self.router.send_multipart([source, DD.bPROTO_VERSION, DD.bCMD_SUBOK, topic_orig.encode(), scopestr.encode()])

        topic = '.'.join([customer, topic])
        topic = topic.encode()
        if source in self.subscriptions:
            self.subscriptions[source].append(topic)
        else:
            self.subscriptions[source] = [topic]
        if topic.decode() in self.topics_trie:
            self.topics_trie[topic.decode()].append(source)
        else:
            self.topics_trie[topic.decode()] = [source]

        logging.info("Got subscription for: %s" % str(topic))
        self.add_sub_south(topic)
        # On_pub_msg should receive a message like : [b'CmdTopic'], where Cmd=\x01 for a sub
        msg = b'\x01' + topic
        self.sub_north_sock.send_multipart([msg])
        self.sub_south_sock.send_multipart([msg])

    def on_router_msg(self, msg):
        """
        Function called automatically when a message is received on the
        socket connected to the router.
        :param msg: Message received from the router
        :return: None
        """
#        print("on_router_msg got: ", msg)

        source = msg.pop(0)
        if source in self.local_cli:
            self.local_cli[source][0] = 0
        elif source in self.local_br:
            self.local_br[source][0] = 0

        tmp = msg.pop(0)
        if tmp != DD.bPROTO_VERSION:
            logging.warning('Different protocols in use, message from: %s' % str(source))
            return
        cmd = msg.pop(0)
        if cmd in self.router_cmds:
            f = self.router_cmds[cmd]
            f(source, msg)
        else:
            logging.warning('Ununderstood command from %s : %d'
                            % (str(source), int.from_bytes(cmd, 'little')))

    def unreg_cli(self, name, args):
        """
        Function called if the command received is UNREG
        Deletes a client previously registered locally.
        :param name: name of the client to delete
        :param args: Frames following the command in the original message
        """
        if name in self.local_cli:
            logging.info(' - Deleting local client %s' % str(name))
            if name in self.subscriptions:
                for topic in self.subscriptions[name]:
                    if self.topic_south[topic] == 1:
                        del self.topics_trie[topic.decode()]
                    self.remove_sub_south(topic)
                    msg = b'\x00' + topic
                    self.sub_north_sock.send_multipart([msg])
                    self.sub_south_sock.send_multipart([msg])

                del self.subscriptions[name]
            self.local_cli.pop(name)
            self.del_cli_up(name)
        else:
            logging.warning('Requested to delete an unknown client')

    def forward(self, source='', args=None):
        """

        :param source:
        :param args:
        :return:
        """
        if not args:
            args = []
        if len(args) < 2:
            return
        src = args.pop(0)
        src_name = args.pop(0)
        dst = args.pop(0)
        if dst in self.local_cli:
            self.forward_locally(dst, src_name, args)
        elif dst in self.dist_cli:
            self.forward_down(src, dst, src_name, args)
        elif self.state == DD.S_ROOT:
            self.dest_invalid([src, dst, args])
        else:
            self.forward_up(src, dst, src_name, args)

    def send(self, source, args):
        """

        :param source:
        :param args:
        :return:
        """
        if len(args) < 1:
            return
        logging.info("Send: source: %s, args: %s" % (source, args))
        customer = self.local_cli[source][1]
        source_name = self.local_cli[source][2].encode()
        is_public = int.from_bytes(args.pop(0), byteorder='little')
        dst = args.pop(0)

        if is_public == 1:
            source_name = '.'.join([customer, source_name.decode()]).encode()
            prefix_dst_name = dst#.decode()
        else:
            prefix_dst_name = '.'.join([customer, dst.decode()])
            prefix_dst_name = prefix_dst_name.encode()

        if prefix_dst_name in self.local_cli:
            print("brokerunsafe forward_locally(", source, ",", prefix_dst_name
                  , ",", source_name, ",", args, ")")
            self.forward_locally(source, prefix_dst_name, source_name, args)
        elif prefix_dst_name in self.dist_cli:
            self.forward_down(source, prefix_dst_name, source_name, args)
        elif self.state == DD.S_ROOT:
            self.dest_invalid([source, prefix_dst_name, args])
        else:
            self.forward_up(source, prefix_dst_name, source_name, args)

    def check_cli_timeout(self):
        """


        """
        tmp_to_del = []
        for client in self.local_cli:
            if self.local_cli[client][0] < 3:
                self.local_cli[client][0] += 1
            else:
                tmp_to_del.append(client)

        for client in tmp_to_del:
            self.unreg_cli(client, [])

    def forward_locally(self, src, src_name, dst, msg):
        """
        Forward message locally
        :param src:
        :param src_name:
        :param dst:
        :param msg:
        """
        #logging.debug('Forward "%s" locally to: %s' % (str(msg), str(self.local_cli[dst][0])))
        self.router.send_multipart([dst,
                                    DD.bPROTO_VERSION,
                                    DD.bCMD_DATA,
                                    src_name,
                                    msg.pop()])

    def forward_down(self, dst_name, source_name, msg):
        """
        Foward message down
        :param dst_name:
        :param source_name:
        :param msg:
        """
        #logging.debug('Forward "%s" to %s through %s' % (str(msg), str(dst_name), str(self.dist_cli[dst_name][0])))
        self.router.send_multipart([self.dist_cli[dst_name][0],
                                    DD.bPROTO_VERSION,
                                    DD.bCMD_FORWARD,
                                    source_name,
                                    dst_name,
                                    msg.pop()])

    def forward_up(self, dst_name, source_name, msg):
        """
        Foward message up
        :param dst_name:
        :param source_name:
        :param msg:
        """
        #logging.debug('Unknown client %s, forwarding up' % dst_name)
        self.dealer.send_multipart([DD.bPROTO_VERSION, DD.bCMD_FORWARD, source_name,
                                    dst_name, msg.pop()])

    #
    # Methods related to the command line inputs
    #

    def on_stdin(self, dummy, other_dummy):
        """

        :param dummy:
        :param other_dummy:
        """
        cmd = sys.stdin.readline().split(maxsplit=2)
        if len(cmd) == 0:
            self.cli_usage()
        elif 'reg' == cmd[0]:
            print('List of local brokers :')
            for b in self.local_br:
                print(b)
            print('List of local clients :')
            for c in self.local_cli:
                print(c)
            print('List of distant clients :')
            for c in self.dist_cli:
                print(c)
        elif 'topics' == cmd[0]:
            self.print_topics()
        elif 'exit' == cmd[0]:
            self.shutdown()
        else:
            self.cli_usage()
        print('---------------')

import argparse
if '__main__' == __name__:
    parser = argparse.ArgumentParser(description='Generic message broker')
    parser.add_argument('name', help='Identity of this broker')
    parser.add_argument('-r',
                        '--router',
                        help='URL to bind ROUTER socket to, e.g. "tcp://*:5555"',
                        nargs='?',
                        default='tcp://*:5555')
    parser.add_argument('-d',
                        '--dealer',
                        help='URL to connect DEALER socket to, "tcp://1.2.3.4:5558"',
                        nargs='?',
                        default='')
    parser.add_argument('-v',
                        '--verbose',
                        action='store_true',
                        help="Make the broker verbose (strongly inadvisalbe with Docker)",
                        default=False)
    parser.add_argument('-p',
                        '--ps',
                        action='store_false',
                        help="Disable pubsub",
                        default=True)
    parser.add_argument(
                        '-u',
                        "--unsafe",
                        action='store_true',
                        help='Unsecure client',
                        default=False)
    parser.add_argument(
                        '-s',
                        "--scope",
                        help='Scope of the broker, e.g. "1/2/3"',
                        nargs='?',
                        default='0/0/0')
    parser.add_argument(
                        '-f',
                        "--logfile",
                        help='File to write logs to',
                        nargs='?',
                        default=None)
    parser.add_argument(
                        '-l',
                        "--loglevel",
                        help='Set loglevel (DEBUG, INFO, WARNING, ERROR, CRITICAL)',
                        nargs='?',
                        default="INFO")
    parser.add_argument(
                        '-k',
                        "--keys",
                        help='Location of the broker keys file',
                        nargs='?',
                        default="broker-keys.json")

    args_list = parser.parse_args()
    numeric_level = getattr(logging, args_list.loglevel.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % args_list.loglevel)

    if args_list.verbose:
        logging.warning("Verbose option is deprecated, use loglevel instead")

    if args_list.logfile:
        logging.basicConfig(format='%(levelname)s:%(message)s',filename=args_list.logfile, level=numeric_level)
    else:
        logging.basicConfig(format='%(levelname)s:%(message)s',filename=args_list.logfile, level=numeric_level)

    if args_list.unsafe==True:
        logging.info("Starting an unsecure broker")
        genbroker = BrokerUnsafe(name=args_list.name.encode('utf8'),
                                                routerurl=args_list.router,
                                                dealerurl=args_list.dealer,
                                                scope=args_list.scope)
    else:
        logging.info("Starting a secure broker")
        genbroker = BrokerSafe(name=args_list.name.encode('utf8'),
                                            routerurl=args_list.router,
                                            dealerurl=args_list.dealer,
                                            scope=args_list.scope,
                                            keys=args_list.keys)

    genbroker.start()
    logging.info("Broker stopped")
