
# inspired from https://gist.github.com/abhinavsingh/6378134

from eventlet.green import zmq
from zmq.eventloop import ioloop
from zmq.eventloop.zmqstream import ZMQStream
ioloop.install()

import tornado.ioloop
import tornado.web
import tornado.websocket
from tornado.ioloop import IOLoop
ioloop = IOLoop.instance()

import os
import logging

from threading import Thread
import multiprocessing
from ryu.lib import hub
import gipc
import time

from tornado.options import define, options, parse_command_line

class MyStaticFileHandler(tornado.web.StaticFileHandler):
    def set_extra_headers(self, path):
        # Disable cache
        self.set_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')

class IndexHandler(tornado.web.RequestHandler):
    def initialize(self, host_ip='localhost', host_port='8888'):
        self.host_ip = host_ip
        self.host_port = host_port

    def get(self):
        self.render("index_template.html", host_ip=self.host_ip, host_port=self.host_port)


class WebSocketHandler(tornado.websocket.WebSocketHandler):

    def initialize(self, socket='ipc:///tmp/log_message'):
        self.socketfile = socket
        # with open("some_image.png", "rb") as imageFile:
        #    self.str = base64.b64encode(imageFile.read())

    def open(self):
        CTX = zmq.Context(1)
        self.socket = zmq.Socket(CTX, zmq.PULL)
        self.socket.bind(self.socketfile)
        self.stream = ZMQStream(self.socket)
        self.stream.on_recv(self.send_data)

        #self.write_message('parsed_nffg.json')
        logging.info('new connection')

    def on_message(self, message):
        #self.write_message('echo:{0}'.format(message))
        logging.info('recv:{0}'.format(message))

    def on_close(self):
        self.stream.close()
        self.socket.close()
        logging.info('connection closed')

    def send_data(self, data):
        self.write_message(data[0])

class NFFG_WebSocketHandler(tornado.websocket.WebSocketHandler):

    def initialize(self, socket='ipc:///tmp/nffg_message'):
        self.socketfile = socket

    def open(self):
        CTX = zmq.Context(1)
        self.socket = zmq.Socket(CTX, zmq.PULL)
        self.socket.bind(self.socketfile)
        self.stream = ZMQStream(self.socket)
        self.stream.on_recv(self.send_data)

        #only send to nffg1 container at start
        if 'nffg1' in self.socketfile:
            self.write_message('parsed_nffg.json')
        logging.info('new connection')

    def on_message(self, message):
        #self.write_message('echo:{0}'.format(message))
        logging.info('recv:{0}'.format(message))

    def on_close(self):
        self.stream.close()
        self.socket.close()
        logging.info('connection closed')

    def send_data(self, data):
        self.write_message(data[0])


class web_server():
    def __init__(self, host_ip, host_port, guest_port):

        #define("port", default=8888, help="run on the given port", type=int)

        self.settings = {
            "static_path": os.path.dirname(__file__),
            "plugin_path": os.path.join(os.path.dirname(__file__), "plugins"),
			"images_path": os.path.join(os.path.dirname(__file__), "images")
        }

        #sender = zmq.Socket(CTX, zmq.PUSH)
        #sender.connect('ipc:///tmp/ws_message')

        #gui_pipe_recv, self.gui_pipe_send = gipc.pipe()
        #self.queue = multiprocessing.Queue()
        # need to use gipc process, other thread types block the main
        p = gipc.start_process(target=self.start_server, args=(), kwargs=dict(host_ip=host_ip, host_port=host_port, guest_port=guest_port))
        #p = multiprocessing.Process(target=self.start_server, args=(self.queue,), kwargs=dict(host_port=host_port))
        #p.start()

    def start_server(self, host_ip='localhost', host_port=10001, guest_port=8888):
        # locally listens to guest_port 8888 (inside docker container)
        # but this is mapped by the orchestrator to host_port (usually 10000)
        app = tornado.web.Application([
            (r'/', IndexHandler, dict(host_ip=host_ip, host_port=host_port)),
            (r'/nffg1', NFFG_WebSocketHandler, dict(socket='ipc:///tmp/nffg1_message')),
            (r'/nffg2', NFFG_WebSocketHandler, dict(socket='ipc:///tmp/nffg2_message')),
            (r'/nffg3', NFFG_WebSocketHandler, dict(socket='ipc:///tmp/nffg3_message')),
            (r'/log', WebSocketHandler, dict(socket='ipc:///tmp/log_message')),
            (r'/count', WebSocketHandler, dict(socket='ipc:///tmp/count_message')),
            (r"/(sigma\.min\.js)", tornado.web.StaticFileHandler,
             dict(path=self.settings['static_path'])),
            (r"/(sigma\.parsers.*)", tornado.web.StaticFileHandler,
             dict(path=self. settings['plugin_path'])),
            (r"/(sigma\.layout.*)", tornado.web.StaticFileHandler,
             dict(path=self.settings['plugin_path'])),
			(r"/images/(.*)", tornado.web.StaticFileHandler,
             dict(path=self.settings['images_path'])),
            (r"/(.*\.json)", MyStaticFileHandler,
             dict(path=self.settings['static_path']))
        ])

        app.listen(guest_port)
        logging.info('start GUI')
        tornado.ioloop.IOLoop.instance().start()



if __name__ == '__main__':
    parse_command_line()
    app.listen(options.port)
    tornado.ioloop.IOLoop.instance().start()