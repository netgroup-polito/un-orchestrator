#!/usr/bin/python3
# -*- coding: utf-8 -*-
import logging
import json
from jsonrpcclient.request import Notification
from jsonrpcserver import Methods, dispatch
from doubledecker.clientSafe import ClientSafe
from cadvproxy.cadv_monitor import CAdvisorMonitorThread

__author__ = 'Umar Toseef'
__email__ = 'umar.toseef@eict.de'

# Based on HTTP error codes
REQUEST_ACCEPTED = 201
REQUEST_NOT_ACCEPTED = 404
CLIENT_STATUS_CONNECTED = 1
CLIENT_STATUS_DISCONNECTED = 2


class DDClient(ClientSafe):

    """
    cAdvisor DoubleDecker client
    """

    def __init__(self, name, dealer_url, customer, key_file, verbose):
        super().__init__(name, dealer_url, customer, key_file)

        self.to_monitor = dict()
        self.STATE = CLIENT_STATUS_DISCONNECTED
        self.verbose = verbose
        self.logger = logging.getLogger("DDClient")

        self.methods = Methods()
        self.methods.add_method(self.start_monitoring)
        self.methods.add_method(self.stop_monitoring)
        self.methods.add_method(self.reset_monitoring)

    def start_monitoring(self, spec_json):
        """
        Starts monitoring a container with given ID and publishes monitoring data on DD bus under topic "measurement"
        :param spec_json: Json specification for measurement as dict. Under the key "parameters" following arguments
        are possible
            container_id: Complete ID of container to be monitored. Please note that short ID is not acceptable.
            interval: Interval in seconds which sets how frequently monitoring data should be published. Default value
            is every 1.0 second.
            tail: Time span over which mean value of a metric is computed. For example, if set to 30 then a mean
            value of past 30 seconds of the metric is reported. Acceptable value range for this argument is from
            1.4 to 90.0 second. Default value is 30 second.
        :return: 201 if provided container id exists and monitoring has been started, otherwise 404.
        """

        self.logger.debug("CALLED start monitoring "+str(spec_json))

        if "parameters" not in spec_json.keys() or "containerID" not in spec_json["parameters"].keys():
            self.logger.debug("No params passed to start_monitoring()")
            return REQUEST_NOT_ACCEPTED

        params = spec_json["parameters"]
        container_id = params["containerID"]
        interval = params["interval"] if "interval" in params.keys() else 1
        tail = params["tail"] if "tail" in params.keys() else 30

        new_thread = CAdvisorMonitorThread(container_id=container_id, interval=interval, ddClient=self, tail=tail,
                                           spec_json=spec_json)

        if container_id in self.to_monitor.keys():
            self.logger.debug("ERROR start_monitoring: Already monitoring: "+str(container_id))
            return REQUEST_NOT_ACCEPTED

        if new_thread.container_exists():
            new_thread.daemon = True
            new_thread.start()
            self.to_monitor[container_id] = new_thread
            self.logger.debug("SUCCESS start monitoring "+str(container_id))
            return REQUEST_ACCEPTED
        else:
            self.logger.debug("FAIL start monitoring "+str(container_id))
            return REQUEST_NOT_ACCEPTED

    def stop_monitoring(self, spec_json):
        """
        Stops monitoring for provided container id
        :param spec_json: Json specification for measurement as dict. Under the key "parameters" following argument
        is considered and the rest is ignored.
            container_id: Complete ID of container for which monitoring should be stopped. Please note that short ID
            is not acceptable.
        :return: 201 if monitoring has been stopped for container with provided ID. 404 if container with provided ID
        was not being monitored.
        """
        self.logger.debug("CALLED stop monitoring "+str(spec_json))

        if "parameters" not in spec_json.keys() or "containerID" not in spec_json["parameters"].keys():
            self.logger.debug("No params passed to start_monitoring()")
            return REQUEST_NOT_ACCEPTED

        params = spec_json["parameters"]
        container_id = params["containerID"]

        if container_id in self.to_monitor.keys():
            self.to_monitor.pop(container_id).event.set()
            self.logger.debug("SUCCESS stop monitoring "+str(container_id))
            return REQUEST_ACCEPTED
        else:
            self.logger.debug("FAIL stop monitoring "+str(container_id))
            return REQUEST_NOT_ACCEPTED

    def reset_monitoring(self):
        """
        Stops monitoring for all containers. No more monitoring data is published on DD bus.
        :return: 201 if operation was successful.
        """
        self.logger.debug("CALLED reset monitoring ")
        for container_id in self.to_monitor:
            self.to_monitor[container_id].event.set()
        self.to_monitor.clear()
        return REQUEST_ACCEPTED

    def on_data(self, src, msg):
        self.logger.debug("Received notification")
        self.handle_jsonrpc(src=src, msg=msg)

    def on_pub(self, src, topic, msg):
        self.logger.debug("Received publication")
        self.handle_jsonrpc(src=src, msg=msg)

    def on_reg(self):
        self.logger.debug("Registered with broker")
        self.STATE = CLIENT_STATUS_CONNECTED

    def on_discon(self):
        self.logger.debug("Disconnected from broker")
        self.STATE = CLIENT_STATUS_DISCONNECTED
        print("on_dis")

    def on_error(self, code, msg):
        self.logger.debug("DD Client ERROR:%s Message:%s" % (str(code), str(msg)))

    def publish_measurement(self, result):
        if not self.STATE == CLIENT_STATUS_DISCONNECTED:
            self.publish(topic="measurement", message=str(Notification("measurement", result=result)))
        if self.verbose:
            print(str(Notification("measurement", result=result))+"\n")

    def handle_jsonrpc(self, src, msg):
        request = json.loads(msg.decode('UTF-8'))

        response = dispatch(self.methods, request)

        # if the http_status is 200, its request/response, otherwise notification
        if response.http_status == 200:
            self.logger.info("Replying to %s with %s" % (str(src), str(response)))
            self.sendmsg(src, str(response))

        # notification, correctly formatted
        elif response.http_status == 204:
            pass

        # if 400, some kind of error
        # return a message to the sender, even if it was a notification
        elif response.http_status == 400:
            self.sendmsg(src, str(response))
            self.logger.error("Received bad JSON-RPC from %s, error %s" % (str(src), str(response)))

        else:
            self.logger.error("Received bad JSON-RPC from %s \nRequest: %s\nResponse: %s"
                          % (str(src), msg.decode(), str(response)))
