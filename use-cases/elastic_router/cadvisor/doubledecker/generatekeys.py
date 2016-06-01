__license__ = """
  Copyright (c) 2015 Pontus Sköldström, Bertrand Pechenot

  This file is part of libdd, the DoubleDecker hierarchical
  messaging system DoubleDecker is free software; you can
  redistribute it and/or modify it under the terms of the GNU Lesser
  General Public License (LGPL) version 2.1 as published by the Free
  Software Foundation.

  As a special exception, the Authors give you permission to link this
  library with independent modules to produce an executable,
  regardless of the license terms of these independent modules, and to
  copy and distribute the resulting executable under terms of your
  choice, provided that you also meet, for each linked independent
  module, the terms and conditions of the license of that module. An
  independent module is a module which is not derived from or based on
  this library.  If you modify this library, you must extend this
  exception to your version of the library.  DoubleDecker is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
  License for more details.  You should have received a copy of the
  GNU Lesser General Public License along with this program.  If not,
  see <http://www.gnu.org/licenses/>.
"""

import binascii
from struct import unpack
from os import urandom
import json
from nacl.public import PrivateKey
from nacl.encoding import Base64Encoder


def generateKeys(names):
    """
    Generate keyfiles for the secure broker
    Writes keys to json files, and returns a tuple with the keys (broker,public,customers)
    :param names: Comma-separated string with customer names ("IBM,Facebook,...")
    or array with names ['IBM','Facebook']
    """

    names_list = list()
    if isinstance(names, str):
        for n in names.split(','):
            nsane = n.strip()
            if nsane != '':
                names_list.append(nsane)
    names = names_list

    brokerKeyList = dict()
    customerKeys = list()
    publicClientKeyList = dict()

    # create one for the brokers, to allow mutual authentication and secret
    # transmission of R
    ddprivkey = PrivateKey.generate()
    ddpubkey = ddprivkey.public_key

    brokerKeyList['dd'] = {
        "pubkey": ddpubkey.encode(
            encoder=Base64Encoder).decode(), 'privkey': ddprivkey.encode(
            encoder=Base64Encoder).decode(), 'R': str(
                unpack(
                    "!Q", urandom(8))[0])}

    # create one for public services, e.g. an orchestrator, monitoring plugin,
    # etc.
    publicprivkey = PrivateKey.generate()
    publicpubkey = publicprivkey.public_key

    brokerKeyList[binascii.hexlify(publicpubkey.__bytes__()).decode()] = {
        "pubkey": publicpubkey.encode(encoder=Base64Encoder).decode(),
        "r": "public",
        "R": str(unpack("!Q", urandom(8))[0])
    }

    publicClientKeyList['public'] = {
        'name': "public", "ddpubkey": ddpubkey.encode(
            encoder=Base64Encoder).decode(),
        "publicpubkey": publicpubkey.encode(
            encoder=Base64Encoder).decode(),
        "pubkey": publicpubkey.encode(
                encoder=Base64Encoder).decode(),
        "privkey": publicprivkey.encode(
                    encoder=Base64Encoder).decode(),
        "hash": binascii.hexlify(
                        publicpubkey.__bytes__()).decode()}

    for n in names:
        privkey = PrivateKey.generate()
        pubkey = privkey.public_key

        brokerKeyList[binascii.hexlify(pubkey.__bytes__()).decode()] = {
            "pubkey": pubkey.encode(encoder=Base64Encoder).decode(),
            "r": n,
            "R": str(unpack("!Q", urandom(8))[0])
        }

        publicClientKeyList[binascii.hexlify(pubkey.__bytes__()).decode()] = {
            "pubkey": pubkey.encode(encoder=Base64Encoder).decode(),
            "r": n}
        # "r": n,
        # "R": unpack("!Q", urandom(8))[0]
        # }
        customerKeys.append(
            {
                'name': n,
                "ddpubkey": ddpubkey.encode(encoder=Base64Encoder).decode(),
                "publicpubkey": publicpubkey.encode(encoder=Base64Encoder).decode(),
                "pubkey": pubkey.encode(encoder=Base64Encoder).decode(),
                "privkey": privkey.encode(encoder=Base64Encoder).decode(),
                "hash": binascii.hexlify(pubkey.__bytes__()).decode()
            }
        )

    f = open("broker-keys.json", 'wb')
    f.write(json.dumps(brokerKeyList, indent=4).encode('utf8'))
    f.close()
    f = open("public-keys.json", 'wb')
    f.write(json.dumps(publicClientKeyList, indent=4) .encode('utf8'))
    f.close()
    for key in customerKeys:
        f = open("%s-keys.json" % key['name'], "wb")
        del key['name']
        f.write(json.dumps(key, indent=4).encode('utf8'))
        f.close()

    return brokerKeyList, publicClientKeyList, customerKeys
