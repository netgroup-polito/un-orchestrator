'''
Created on Sep 29, 2014

@author: fabiomignini
'''
import logging
import json
import inspect
import os

from jsonschema import validate, ValidationError

from .exception import NF_FGValidationError

class ValidateNF_FG(object):
    '''
    classdocs
    '''
    schema_name = 'schema.json'
    
    def validate(self, nffg):
        schema = self.get_schema()
        try:
            validate(nffg, schema)
        except ValidationError as err:
            logging.info(err.message)
            raise NF_FGValidationError(err.message)
        
    def get_schema(self):
        base_folder = os.path.realpath(os.path.abspath(os.path.split(inspect.getfile( inspect.currentframe() ))[0]))
        fd = open(base_folder+'/'+self.schema_name, 'r')
        return json.loads(fd.read())