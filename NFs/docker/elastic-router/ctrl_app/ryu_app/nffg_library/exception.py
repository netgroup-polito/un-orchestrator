'''
Created on Oct 20, 2015

@author: fabiomignini
'''

class InexistentLabelFound(Exception):
    def __init__(self, message):
        self.message = message
        # Call the base class constructor with the parameters it needs
        super(InexistentLabelFound, self).__init__(message)
    
    def get_mess(self):
        return self.message
    
class WrongNumberOfPorts(Exception):
    def __init__(self, message):
        self.message = message
        # Call the base class constructor with the parameters it needs
        super(WrongNumberOfPorts, self).__init__(message)
    
    def get_mess(self):
        return self.message
    
class NF_FGValidationError(Exception):
    def __init__(self, message):
        self.message = message
        # Call the base class constructor with the parameters it needs
        super(NF_FGValidationError, self).__init__(message)
    
    def get_mess(self):
        return self.message