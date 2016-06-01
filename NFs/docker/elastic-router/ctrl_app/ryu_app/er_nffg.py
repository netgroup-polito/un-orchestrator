import urllib2
import logging
logging.basicConfig(level=logging.DEBUG)

def get_nffg_xml(base_url):
    url = base_url + '/get-config'
    #print url
    # by default a GET request is created:
    req = urllib2.Request(url, '')
    nffg_xml = urllib2.urlopen(req).read()
    return nffg_xml


def get_nffg_json(base_url):
    url = base_url + '/NF-FG/NF-FG'
    #print url
    # by default a GET request is created:
    req = urllib2.Request(url)
    nffg_json = urllib2.urlopen(req).read()
    return nffg_json


def send_nffg_xml(base_url, xml_nffg):
    url = base_url + '/edit-config'
    req = urllib2.Request(url, xml_nffg)
    response = urllib2.urlopen(req)
    result = response.read()
    logging.info(result)


def send_nffg_json(base_url, nffg_json):
    url = base_url + '/NF-FG/NF-FG'
    req = urllib2.Request(url, nffg_json)
    req.get_method = lambda: 'PUT'
    req.add_header("Content-Type", "application/json")
    response = urllib2.urlopen(req)
    result = response.read()
    logging.info(result)

# define functions to process the nffg (xml or json format)
try:
    nffg_env = os.environ['NFFG']
    if nffg_env == 'json':
        from json_er import *
        get_nffg = get_nffg_json
        send_nffg = send_nffg_json
    elif nffg_env == 'xml':
        from xml_er import *
        get_nffg = get_nffg_xml
        send_nffg = send_nffg_xml
except:
    from json_er import *
    get_nffg = get_nffg_json
    send_nffg = send_nffg_json