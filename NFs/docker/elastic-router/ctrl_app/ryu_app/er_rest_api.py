from flask import Flask
from flask_restful import Resource, Api

from eventlet import wsgi
import eventlet
import time

from ryu.lib import hub

import logging
logging.basicConfig(level=logging.DEBUG)


app = Flask(__name__)
api = Api(app)

class er_rest_api():


    def __init__(self):


        self.log = logging.getLogger('rest_api_logger')
        self.log.setLevel(logging.DEBUG)


        self.function_dict = {
            'in': self.scale_in,
            'out': self.scale_out,
            'ping': self.ping
             }

    def get(self):
        return 'test'

    def scale_in(self):
        start_time = time.time()
        scaling_ports = self.er_monitor.start_scale_in_default()
        self.log.debug('start scale in')
        if len(scaling_ports) > 0:
            self.ER_app.VNFs_to_be_deleted = self.ER_app.scale(scaling_ports, 'in')
            self.er_monitor.scaling_lock.acquire()
            self.er_monitor.scaling_lock.release()
        else:
            return 'scaling in not possible'
        scaling_time = time.time() - start_time
        return 'scaling in finished ({0} seconds)'.format(round(scaling_time,2))

    def scale_out(self):
        start_time = time.time()
        scaling_ports = self.er_monitor.start_scale_out_default()
        self.log.debug('start scale out')
        if len(scaling_ports) > 0:
            self.ER_app.VNFs_to_be_deleted = self.ER_app.scale(scaling_ports, 'out')
            self.er_monitor.scaling_lock.acquire()
            self.er_monitor.scaling_lock.release()
        else:
            return 'scaling out not possible'
        scaling_time = time.time() - start_time
        return 'scaling out finished ({0} seconds)'.format(round(scaling_time, 2))

    def ping(self):
        return 'pong'

    def start_rest_server(self, monitor_instance, ER_instance, host='0.0.0.0', port=5000):

        # monitoring class
        self.er_monitor = monitor_instance
        # Elastic Router class
        self.ER_app = ER_instance

        # in Ryu we must always use the eventlet threads, otherwise normal controller functionality will break.
        rest_api = hub.spawn(wsgi.server, eventlet.listen((host, port)), app)

        logging.info('started REST API')
        return rest_api


rest_api = er_rest_api()


class rest_calls(Resource):
    def get(self, scale_arg):
        return rest_api.function_dict[scale_arg]()


api.add_resource(rest_calls, '/scale/<scale_arg>')
