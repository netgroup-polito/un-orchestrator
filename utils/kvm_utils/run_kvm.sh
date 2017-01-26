#!/bin/bash
sudo service libvirt-bin stop
sudo /usr/local/sbin/libvirtd --daemon
