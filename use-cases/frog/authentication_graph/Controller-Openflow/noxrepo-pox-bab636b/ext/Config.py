import ConfigParser

class Configuration(object):
    
    _instance = None
    #(fmignini) Not too meaningful use this var, I should change his name with something else like inizialized = False 
    _AUTH_SERVER = None
    
    def __new__(cls, *args, **kwargs):
        
        if not cls._instance:
            cls._instance = super(Configuration, cls).__new__(
                                cls, *args, **kwargs)
        return cls._instance 
    
    def __init__(self):
        if self._AUTH_SERVER is None:
            self.inizialize()
    
    def inizialize(self): 
        config = ConfigParser.RawConfigParser()
        config.read('controller.conf')
        self._my_infos = lambda: None
        self._my_infos.ip = config.get('my_infos', 'ip')
        self._my_infos.tcp_port = int(config.get('my_infos', 'tcp_port'))
        #self._my_infos.my_default_GRE_tunnel = int(config.get('my_infos', 'my_default_GRE_tunnel'))
        self._my_infos.not_user_ports = list(config.get('my_infos', 'not_user_ports'))
        self._cp = lambda: None
        self._cp.ip = config.get('captive_portal_infos', 'ip')
        self._cp.tcp_port = int(config.get('captive_portal_infos', 'tcp_port'))
        self._cp.mac = config.get('captive_portal_infos', 'mac_address')
        self._cp.switch_port = int(config.get('captive_portal_infos', 'switch_port'))
        self._LOG_FILE = config.get('log', 'log_file')
        self._VERBOSE = config.get('log', 'verbose')
        self._DEBUG = config.get('log', 'debug')

        
    @property
    def my_infos(self):
        return self._my_infos
        
    @property
    def captive_portal(self):
        return self._cp

    @property
    def LOG_FILE(self):
        return self._LOG_FILE

    @property
    def VERBOSE(self):
        return self._VERBOSE

    @property
    def DEBUG(self):
        return self._DEBUG

