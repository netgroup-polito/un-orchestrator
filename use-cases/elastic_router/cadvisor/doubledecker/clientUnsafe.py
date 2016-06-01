import logging
import sys
import abc
import re

import zmq
import zmq.eventloop.ioloop
import zmq.eventloop.zmqstream

from . import proto as DD
from . import clientInterface as interface


class ClientUnsafe(interface.Client):
    def __init__(self, name, dealerurl, customer):
        super().__init__(name, dealerurl, customer)
        self._dealer.setsockopt(zmq.IDENTITY, self._customer_name)
        self._hash = b'unsafe'
        self._safe = False

    @abc.abstractmethod
    def on_reg(self):
        pass

    def on_cli(self, dummy, other_dummy):
        cmd = sys.stdin.readline().split(maxsplit=2)
        if len(cmd) == 0:
            self._cli_usage()
        elif 'send' == cmd[0]:
            if len(cmd) < 3:
                self._cli_usage()
                return
            dst = cmd[1].encode("utf8")
            msg = cmd[2].strip().encode('utf8')
            logging.debug("Sending \"%s\" to %s" %
                          (msg, dst.decode('utf8')))
            self.sendmsg(dst, msg)
        elif 'exit' == cmd[0]:
            self.shutdown()
        elif 'sub' == cmd[0]:
            if len(cmd) > 2:
                # print("Subscribing to",cmd[1]+'/'+cmd[2])
                self.sub_scope(cmd[1], cmd[2])
            else:
                print("usage: sub [topic] [scope], where scope is ALL, REGION, CLUSTER, NODE, 1/2/3, NOSCOPE")
        elif 'unsub' == cmd[0]:
            if len(cmd) > 1:
                if (cmd[1] in self._sublist):
                    logging.info("Unsubscribing from %s*" % cmd[1])
                    for sub in self._sublist:
                        if(sub.startswith(cmd[1]+"/")):
                            self._dealer.send_multipart([DD.bPROTO_VERSION, DD.bCMD_UNSUB, sub.encode()])
                            self._sublist.remove(sub)

                else:
                    logging.info("You are not subscribed to this topic: %s" % cmd[1])
            else:
                print("usage: unsub [topic]")
        elif 'pub' == cmd[0]:
            if len(cmd) > 2:
                logging.info("Publishing message on %s*" % cmd[1])
                self.publish(cmd[1].encode(), cmd[2].encode())
            else:
                print("usage: pub [topic] [message]")
        else:
            self._cli_usage()

    def subscribe(self, topic, scope):
        scope = scope.strip().lower()
        if scope == 'all':
            scopestr = "/"
        elif scope == 'region':
            scopestr = "/*/"
        elif scope == "cluster":
            scopestr = "/*/*/"
        elif scope == "node":
            scopestr = "/*/*/*/"
        elif scope == "noscope":
            scopestr = "noscope"
        elif re.match("/(\d/)+", scope):
            # check that scope only contains numbers and slashes
            if scope[-1] == '/':
                scopestr = scope
            else:
                scopestr = scope + '/'
        else:
            print("The scope should be like: '/1/2/3/'")
            return
        if scopestr == "noscope":
            logging.info("Subscribing to %s" % topic)
        else:
            logging.info("Subscribing to %s" % (topic + scopestr))
        self._send(DD.bCMD_SUB, [topic.encode(), scopestr.encode()])

    def publish(self, topic, message):
        if self._state != DD.S_REGISTERED:
            raise (ConnectionError("Not registered"))
        if type(topic) is str:
            topic = topic.encode('utf8')
        if type(message) is str:
            message = message.encode('utf8')

        self._dealer.send_multipart([DD.bPROTO_VERSION, DD.bCMD_PUB, b'0', topic, b'', message])

    def publish_public(self, topic, message):
        if self._state != DD.S_REGISTERED:
            raise (ConnectionError("Not registered"))
        if type(topic) is str:
            topic = topic.encode('utf8')
        if type(message) is str:
            message = message.encode('utf8')

        self._dealer.send_multipart([DD.bPROTO_VERSION, DD.bCMD_PUB, b'1', topic, b'', message])

    def sendmsg(self, dst, msg):
        if self._state != DD.S_REGISTERED:
            raise (ConnectionError("Not registered"))
        if type(dst) is str:
            dst = dst.encode('utf8')
        if type(msg) is str:
                msg = msg.encode('utf8')

        logging.info("client-sendmsg called!\n")
        if self._customer == b'public':
            dst_is_public = True
            try:
                split = dst.split('.')
                # customer_dst = split[0]
            except:
                pass
            if dst_is_public:
                # public --> public
                self._send(DD.bCMD_SEND, [b'0', dst, msg])
            else:
                # public --> non-public
                self._send(DD.bCMD_SEND, [b'1', dst, msg])
        else:
            # send to a public or not ?
            dst_is_public = False
            try:
                split = dst.split('.')
                dst_is_public = split[0] == 'public'
            except:
                pass

            if dst_is_public:
                # non-public --> public
                self._send(DD.bCMD_SEND, [b'1', dst, msg])
            else:
                # non-public --> non-public
                self._send(DD.bCMD_SEND, [b'0', dst, msg])

    def _ask_registration(self):
        self._dealer.setsockopt(zmq.LINGER, 0)
        self._stream.close()
        self._dealer.close()
        self._dealer = self._ctx.socket(zmq.DEALER)
        self._dealer.setsockopt(zmq.IDENTITY, self._customer_name)
        self._dealer.connect(self._dealerurl)
        self._stream = zmq.eventloop.zmqstream.ZMQStream(self._dealer, self._IOLoop)
        self._stream.on_recv(self._on_message)
        self._send(DD.bCMD_ADDLCL, [self._hash, self._customer, self._name])

    def _on_message(self, msg):
        self._timeout = 0
        if msg.pop(0) != DD.bPROTO_VERSION:
            logging.warning('different protocols in use, message discarded')
            return
        cmd = msg.pop(0)
        if cmd == DD.bCMD_REGOK:
            self._state = DD.S_REGISTERED
            self._register_loop.stop()
            try:
                scope = msg.pop(0)
                self.scope = scope
            except:
                pass
            self._heartbeat_loop.start()
            self._send(DD.bCMD_PING)
            self.on_reg()
        elif cmd == DD.bCMD_DATA:
            # print(msg.pop(0), 'sent', msg)
            self.on_data(msg.pop(0), msg)
        elif cmd == DD.bCMD_PONG:
            ioloop = zmq.eventloop.ioloop.IOLoop.current()
            ioloop.add_timeout(ioloop.time() + 1.5, self._ping)
        elif cmd == DD.bCMD_PUB:
            src = msg.pop(0)
            topic = msg.pop(0)
            self.on_pub(src, topic, msg)
        elif cmd == DD.bCMD_SUBOK:
            topic = msg.pop(0).decode()
            scope = msg.pop(0).decode()
            tt = "%s%s" % (topic, scope)
            if tt not in self._sublist:
                self._sublist.append(tt)
            else:
                logging.error("You are already subscribed to this topic")
                self._dealer.send_multipart(
                    [DD.bPROTO_VERSION,
                     DD.bCMD_UNSUB, topic.encode()])
        elif cmd == DD.bCMD_NODST:
            logging.warning("Unknown client %s" % msg.pop(0))
        else:
            logging.warning("Unknown command, got: %d %s" % (cmd, msg))

    def _cli_usage(self):
        print("Commands: ")
        print("help                     - show this help")
        print("send  [client] [message] - send message to client")
        print("pub   [topic]  [message] - publish message on topic")
        print("sub   [topic]            - subscribe to messages in topic")
        print("unsub [topic]            - subscribe to messages in topic")
        print('exit                     - unregister and exit')
