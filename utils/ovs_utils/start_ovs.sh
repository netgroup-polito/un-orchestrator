#!/bin/bash
sudo /usr/share/openvswitch/scripts/ovs-ctl start
sudo ovs-appctl -t ovsdb-server ovsdb-server/add-remote ptcp:6632
