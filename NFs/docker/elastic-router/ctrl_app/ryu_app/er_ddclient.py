__author__ = 'Administrator'

from multiprocessing import Process

from ryu.lib import hub
import logging
from doubledecker.clientSafe import ClientSafe
logging.basicConfig(level=logging.DEBUG)
import time
import json
from jsonrpcserver import dispatch, Methods
import gipc


class er_ddclient:
    def __init__(self, ER_monitor, ER_instance):
        self.log = logging.getLogger('zmq_logger')
        self.log.setLevel(logging.DEBUG)

        # monitoring class
        self.ER_monitor = ER_monitor
        # Elastic Router class
        self.ER_app = ER_instance

        # we need the gpic module to be compatible with gevent threads
        readend, writeend = gipc.pipe()

        # process the received message, receive from other process
        sub_server = hub.spawn(self.receive_data, readend)

        # send received DD messages (ddclient only works as Process, other type of threads are blocking the main thread)
        p = gipc.start_process(target=self.alarm_receiver, args=(writeend,))

    def receive_data(self, conn):
        while True:
            # wait for message
            message = conn.get()

            start_time = time.time()

            if 'scale_in' in message:
                scaling_ports = self.ER_monitor.start_scale_in_default()
                logging.info('start scale in')
                if len(scaling_ports) > 0:
                    self.ER_app.VNFs_to_be_deleted = self.ER_app.scale(scaling_ports, 'in')
                    # wait unit scaling lock is released
                    self.ER_monitor.scaling_lock.acquire()
                    self.ER_monitor.scaling_lock.release()

            elif 'scale_out' in message:
                scaling_ports = self.ER_monitor.start_scale_out_default()
                logging.info('start scale out')
                if len(scaling_ports) > 0:
                    self.ER_app.VNFs_to_be_deleted = self.ER_app.scale(scaling_ports, 'out')
                    # wait unit scaling lock is released
                    self.ER_monitor.scaling_lock.acquire()
                    self.ER_monitor.scaling_lock.release()

            scaling_time = time.time() - start_time
            logging.info('scaling finished ({0} seconds)'.format(round(scaling_time, 2)))


    def alarm_receiver(self, pipe):
        DDclient = DD_client(
            name='ryu',
            dealerurl='tcp://172.17.0.1:5555',
            keyfile='./a-keys.json',
            output=pipe
        )

        self.log.info("Subscribe DDclient")
        DDclient.alarm_sub(b'sub', b'monitor_alarm', b'node')

        self.log.info("Start DDclient")
        DDclient.start()


class DD_client(ClientSafe):
    def __init__(self, name, dealerurl, keyfile, output):
        super(DD_client, self).__init__(name, dealerurl, keyfile)
        self.subscriptions = []
        self.registered = False

        self.output = output

        self.methods = Methods()
        self.methods.add_method(self.alarms)

    # callback called upon registration of the client with its broker
    def on_reg(self):
        logging.info("The client is now connected")
        self.registered = True
        for topic_, scope_ in self.subscriptions:
            self.subscribe(topic_, scope_)

    # callback called when the client detects that the heartbeating with
    # its broker has failed, it can happen if the broker is terminated/crash
    # or if the link is broken
    def on_discon(self):
        logging.warning("The client got disconnected")
        self.registered = False

    # callback called when the client receives an error message
    def on_error(self, code, msg):
        logging.error("ERROR n#%d : %s" % (code, msg))

    def on_pub(self, src, topic, msg):
        self.handle_jsonrpc(src=src, msg=msg, topic=topic)

    def on_data(self, src, msg):
        self.handle_jsonrpc(src, msg, topic=None)

    def handle_jsonrpc(self, src, msg, topic=None):
        request = json.loads(msg.decode('UTF-8'))

        if 'error' in request:
            logging.error(str(request['error']))
            return

        if 'result' in request:
            logging.info(str(request['result']))
            return

        # include the 'ddsrc' parameter so the
        # dispatched method knows where the message came from
        if 'params' not in request:
            request['params'] = {}

        request['params']['ddsrc'] = src.decode()
        response = dispatch(self.methods, request)

        # if the http_status is 200, its request/response, otherwise notification
        if response.http_status == 200:
            logging.info("Replying to %s with %s" % (str(src), str(response)))
            self.sendmsg(src, str(response))
        # notification, correctly formatted
        elif response.http_status == 204:
            pass
        # if 400, some kind of error
        # return a message to the sender, even if it was a notification
        elif response.http_status == 400:
            self.sendmsg(src, str(response))
            logging.error("Received bad JSON-RPC from %s, error %s" % (str(src), str(response)))
        else:
            logging.error(
                "Received bad JSON-RPC from %s \nRequest: %s\nResponose: %s" % (str(src), msg.decode(), str(response)))

    def alarms(self, ddsrc, message):
        logging.info("Received ALARM: {0}".format(message))

        # send message to other thread/process
        self.output.put(message)
        return


    def alarm_sub(self, action, topic, scope):

        action_ = action
        topic_ = topic.decode()
        scope_ = scope.decode()

        if b'sub' == action_:
            self.subscriptions.append((topic_, scope_))
            if self.registered:
                self.subscribe(topic_, scope_)
        elif b'unsub' == action_:
            try:
                self.subscriptions.remove((topic_, scope_))
            except ValueError as e:
                logging.error("Ryu tried to unsubscribe from a non-existing subscription")
            if self.registered:
                self.unsubscribe(topic_, scope_)
        else:
            logging.error("Ryu sent a weird command :", action_)