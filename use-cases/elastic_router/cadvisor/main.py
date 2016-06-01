#!/usr/bin/python3
# -*- coding: utf-8 -*-
import logging

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

import argparse
from cadvproxy.cadv_ddclient import DDClient


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="CAdvisor Aggregator DoubleDecker Client")
    parser.add_argument('name', help="Identity of this client")
    parser.add_argument('customer', help="Name of the customer to get the keys (i.e. 'a' for the customer-a.json file)")
    parser.add_argument(
        '-d',
        "--dealer",
        help='URL to connect DEALER socket to, "tcp://1.2.3.4:5555"',
        nargs='?',
        default='tcp://127.0.0.1:5555')
    parser.add_argument(
        '-f',
        "--logfile",
        help='File to write logs to',
        nargs='?',
        default='log.txt')
    parser.add_argument(
        '-l',
        "--loglevel",
        help='Set loglevel (DEBUG, INFO, WARNING, ERROR, CRITICAL)',
        nargs='?',
        default="INFO")
    parser.add_argument(
        '-k',
        "--keyfile",
        help='File containing the encryption/authentication keys)',
        nargs='?',
        default='')
    parser.add_argument('-c',
                        '--container',
                        help='Name of the container to monitor',
                        default=None)

    parser.add_argument('-v',
                        '--verbose',
                        help='Send copies of measurement messages to console',
                        action='store_true')

    args = parser.parse_args()

    numeric_log_level = getattr(logging, args.loglevel.upper(), None)
    if not isinstance(numeric_log_level, int):
        raise ValueError('Invalid log level: %s' % args.loglevel)
    logging.basicConfig(filename=args.logfile, level=numeric_log_level, format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
    logging.getLogger("urllib3").setLevel(logging.WARNING)

    client = DDClient(
        name=args.name,
        dealer_url=args.dealer,
        customer=args.customer,
        key_file=args.keyfile,
        verbose=args.verbose,
    )
    try:
        if args.container is not None:
            client.start_monitoring(args.container, 1, 60)
        client.start()
    except KeyboardInterrupt as e:
        print("\n--KeyboardInterrupt--")
        client.reset_monitoring()
        try:
            client.shutdown()
        except ValueError:
            pass
