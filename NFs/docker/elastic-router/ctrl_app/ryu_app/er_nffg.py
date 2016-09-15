import urllib2
import logging
import os
logging.basicConfig(level=logging.DEBUG)

#from xml_er import remove_measure_to_ovs_vnfs
def get_nffg_xml(base_url):
    url = base_url + '/get-config'
    logging.info('url: {0}'.format(url))
    #print url
    # by default a GET request is created:
    req = urllib2.Request(url, '')
    nffg_xml = urllib2.urlopen(req).read()
    nffg_xml = remove_measure_to_ovs_vnfs(nffg_xml)
    return nffg_xml


def get_nffg_json(base_url):
    #url = base_url + '/NF-FG/NF-FG'
    url = base_url + '/NF-FG/1'
    #print url
    # by default a GET request is created:
    req = urllib2.Request(url)
    nffg_json = urllib2.urlopen(req).read()
    return nffg_json


def send_nffg_xml(base_url, xml_nffg):
    url = base_url + '/edit-config'
    logging.info('url: {0}'.format(url))
    req = urllib2.Request(url, xml_nffg)
    response = urllib2.urlopen(req)
    result = response.read()
    #logging.info(result)
    return result


def send_nffg_json(base_url, nffg_json):
    url = base_url + '/NF-FG/NF-FG'
    url = base_url + '/NF-FG/1'
    req = urllib2.Request(url, nffg_json)
    req.get_method = lambda: 'PUT'
    req.add_header("Content-Type", "application/json")
    response = urllib2.urlopen(req)
    result = response.read()
    logging.info(result)

# define functions to process the nffg (xml or json format)
try:
    nffg_env = os.environ['NFFG_FORMAT']
    if nffg_env == 'json':
        logging.info('JSON based NFFG')
        from json_er import *
        get_nffg = get_nffg_json
        send_nffg = send_nffg_json
    elif nffg_env == 'xml':
        logging.info('XML based NFFG')
        from xml_er import *
        get_nffg = get_nffg_xml
        send_nffg = send_nffg_xml
except:
    from xml_er import *
    get_nffg = get_nffg_xml
    send_nffg = send_nffg_xml