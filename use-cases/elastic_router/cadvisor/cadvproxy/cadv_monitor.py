#!/usr/bin/python3
# -*- coding: utf-8 -*-
import logging

import threading
import requests
import datetime

__license__ = """
    This file is part of HMD (Hierarchical MajorDomo).

    HMD is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    HMD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MDP.  If not, see <http://www.gnu.org/licenses/>.
"""

__author__ = 'Umar Toseef'
__email__ = 'umar.toseef@eict.de'

class CAdvisorMonitorThread(threading.Thread):
    """
    Periodically calls REST API method of locally running cadvisor
    """

    def __init__(self, container_id, interval, ddClient, tail, spec_json):
        threading.Thread.__init__(self)
        self.container_id = container_id
        self.interval = interval
        self.ddClient = ddClient
        self.spec_json = spec_json
        # Monitoring data from cadvisor comes in steps of ~1.4 sec
        count = tail/1.4 + 1
        if count < 2: # Minimum 2 steps are needed to get an average value over 1.4 sec
            count = 2
        if count > 64: # Maximum 64 steps (~92 sec) are supported by the method
            count = 64
        self.url = ('http://localhost:8080/api/v2.0/stats/' +
                    container_id + '?type=docker&count='+str(round(count)))
        self.event = threading.Event()
        self.lock = threading.Lock()
        self.logger = logging.getLogger("MonitorThread")

    def run(self):
        self.logger.debug("Monitoring URL: "+self.url)
        while not self.event.is_set():
            self.monitor()
            self.event.wait(self.interval)

    def container_exists(self):
        try:
            r = requests.get(self.url)
            if r.status_code != 200:
                return False
        except requests.exceptions.RequestException:
            return False

        return True

    def monitor(self):
        """
        Get cAdvisor statistics via its RESTful API, then publish cpu[%] and
        memory[megabyte] usage on the message bus.
        """

        try:
            r = requests.get(self.url)
            if r.status_code != 200:
                self.logger.error("error:", r.status_code)
                return None

            j = r.json()

            for i in j:
                # memory used by the container in megabytes
                mem_usage = int(j.get(i)[0]['memory']['usage']) / 1e6

                cpu_usage_now = j.get(i)[0]['cpu']['usage']['total']
                cpu_usage_prev = j.get(i)[-1]['cpu']['usage']['total']
                timestamp_now = _convert_timestamp_to_diff(
                    j.get(i)[0]['timestamp'])
                timestamp_prev = _convert_timestamp_to_diff(
                    j.get(i)[-1]['timestamp'])

                # cpu usage percentage (adjusting the cores)
                core_count = len(j.get(i)[0]['cpu']['usage']['per_cpu_usage']) if 'per_cpu_usage' in j.get(i)[0]['cpu']['usage'] else 1

                cpu_load = (float(cpu_usage_now - cpu_usage_prev) /
                            float(timestamp_now - timestamp_prev)) / core_count * 100
                if cpu_load > 100:
                    cpu_load = 100.0
                if cpu_load < 0:
                    cpu_load = 0.0

                self.spec_json["results"]["ram"] = mem_usage
                self.spec_json["results"]["cpu"] = cpu_load

                # Handling thread synchronization
                with self.lock:
                    self.ddClient.publish_measurement(self.spec_json)

        except requests.exceptions.RequestException:
            self.logger.error("RequestException when connecting to cAdvisor")
            return None


def _convert_timestamp_to_diff(timestamp):
    """
    Convert cAdvisor timestamps to nanoseconds for the sake of
    timestamp comparation.
    """
    ts_s = int(datetime.datetime.strptime(
        timestamp.split('.')[0], "%Y-%m-%dT%H:%M:%S").strftime("%s"))
    ts_ns = int(timestamp.split('.')[1][:-1])
    t = [a*b for a, b in zip([1e9, 1], [ts_s, ts_ns])]
    return t[0] + t[1]
