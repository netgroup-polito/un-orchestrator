

import logging
from TakeMac import TakeMac
import falcon
import subprocess



logging.basicConfig(filename="TakeMac.log",level=logging.DEBUG,format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
logging.debug("TakeMac Starting")

bashCommand = TakeMac.ifconfig + TakeMac.interface + TakeMac.pipe
process = subprocess.Popen(bashCommand, stdout=subprocess.PIPE, shell=True)
mac_address,err = process.communicate()
logging.debug("The received MAC is " + str(mac_address)[:-1])
# logging.debug("The error is " + str(err)) 

app = falcon.API()
mac = TakeMac(mac_address[:-1])
app.add_route('/mac', mac)