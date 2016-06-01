'''
Created on Apr 5, 2016

@author: stefanopetrangeli
'''

class ClientError(Exception):
    def __init__(self, message):
        self.message = message
        # Call the base class constructor with the parameters it needs
        super(ClientError, self).__init__(message)
    
    def get_mess(self):
        return self.message
    
class ServerError(Exception):
    def __init__(self, message):
        self.message = message
        # Call the base class constructor with the parameters it needs
        super(ServerError, self).__init__(message)
    
    def get_mess(self):
        return self.message