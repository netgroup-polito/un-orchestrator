'''
Created on Dec 18, 2015

@author: fabiomignini
'''
import os
import netifaces
import logging
import json
import xmltodict

try:
    import StringIO
except ImportError:
    from io import StringIO

from pyang.__init__ import Context, FileRepository
from pyang.translators.yin import YINPlugin
from pyang import plugin

class Bash():
    def __init__(self, command):
        command = command + " 2>&1"
        self.pipe = os.popen(command)
        self.output = self.pipe.read()
        self.exit_code = self.pipe.close()
        if self.exit_code is not None:
            logging.debug ("Error code: "+str(self.exit_code)+" -Due to command: "+str(command)+" - Message: "+self.output)
    
    def get_output(self):
        return self.output
    
def get_mac_address(configuration_interface):
    interfaces = netifaces.interfaces()
    for interface in interfaces:
        if interface == configuration_interface:
            return netifaces.ifaddresses(interface)[17][0]['addr']

def _transform_yang_to_dict(yang_model_string): 
    class Opts(object):
        def __init__(self, yin_canonical=False, yin_pretty_strings=True):
            self.yin_canonical = yin_canonical
            self.yin_pretty_strings = yin_pretty_strings
    ctx = Context(FileRepository())
    yang_mod =  ctx.add_module('yang', yang_model_string, format='yang')
    
    yin = YINPlugin()
    modules = []
    modules.append(yang_mod)
    ctx.opts = Opts()
    yin_string = StringIO()
    yin.emit(ctx=ctx, modules=modules, fd=yin_string)
    xml = yin_string.getvalue()
    return xmltodict.parse(xml)
       
def validate_json(json_instance, yang_model):
    yang_model_dict = _transform_yang_to_dict(yang_model)
    working_dir = '/tmp/'
    Bash('cd '+working_dir+' \n cp /usr/local/share/yang/modules/ietf/* .')

    # Save temporary on disk the xml and the yang model
    yang_file_name = yang_model_dict['module']['@name']+".yang"
    yang_name = yang_model_dict['module']['@name']
    with open (working_dir+yang_file_name, "w") as yang_model_file:
        yang_model_file.write(yang_model)
    json_file_name = yang_name+".json"
    with open (working_dir+json_file_name, "w") as yang_model_file:
        yang_model_file.write(json_instance)

    Bash('cd '+working_dir+' \n yang2dsdl -t config '+yang_file_name)
    Bash('cd '+working_dir+' \n pyang -f jtox -o '+yang_name+'.jtox '+yang_name+'.yang')
    Bash('cd '+working_dir+' \n json2xml -t config -o '+yang_name+'.xml '+yang_name+'.jtox '+yang_name+'.json')
    response = Bash('cd /tmp/ \n yang2dsdl -s -j -b '+yang_name+' -t config -v '+yang_name+'.xml')
    return response.exit_code,  response.get_output()