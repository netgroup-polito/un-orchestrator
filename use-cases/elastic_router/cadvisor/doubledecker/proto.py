# coding=utf-8
__author__ = 'eponsko'
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
# Double-decker definitions
proto_version = 0x0d0d0003
CMD_SEND = 0
CMD_FORWARD = 1
CMD_PING = 2
CMD_ADDLCL = 3
CMD_ADDDCL = 4
CMD_ADDBR = 5
CMD_UNREG = 6
CMD_UNREGDCLI = 7
CMD_UNREGBR = 8
CMD_DATA = 9
CMD_ERROR = 10
CMD_REGOK = 11
CMD_PONG = 12
CMD_CHALL = 13
CMD_CHALLOK = 14
CMD_PUB = 15
CMD_SUB = 16
CMD_UNSUB = 17
CMD_SENDPUBLIC = 18
CMD_PUBPUBLIC = 19
CMD_SENDPT = 20
CMD_FORWARDPT = 21
CMD_DATAPT = 22
CMD_SUBOK = 23

S_UNREG = 1
S_ROOT = 2
S_EXIT = 3
S_CHALLENGED = 4
S_REGISTERED = 5


# byte packages
bPROTO_VERSION = int.to_bytes(proto_version, length=4, byteorder='little')
bCMD_SEND = int.to_bytes(CMD_SEND, length=4, byteorder='little')
bCMD_FORWARD = int.to_bytes(CMD_FORWARD, length=4, byteorder='little')
bCMD_PING = int.to_bytes(CMD_PING, length=4, byteorder='little')
bCMD_ADDLCL = int.to_bytes(CMD_ADDLCL, length=4, byteorder='little')
bCMD_ADDDCL = int.to_bytes(CMD_ADDDCL, length=4, byteorder='little')
bCMD_ADDBR = int.to_bytes(CMD_ADDBR, length=4, byteorder='little')
bCMD_UNREG = int.to_bytes(CMD_UNREG, length=4, byteorder='little')
bCMD_UNREGBR = int.to_bytes(CMD_UNREGBR, length=4, byteorder='little')
bCMD_UNREGDCLI = int.to_bytes(CMD_UNREGDCLI, length=4, byteorder='little')
bCMD_DATA = int.to_bytes(CMD_DATA, length=4, byteorder='little')
bCMD_ERROR = int.to_bytes(CMD_ERROR, length=4, byteorder='little')
bCMD_REGOK = int.to_bytes(CMD_REGOK, length=4, byteorder='little')
bCMD_PONG = int.to_bytes(CMD_PONG, length=4, byteorder='little')
bCMD_CHALL = int.to_bytes(CMD_CHALL, length=4, byteorder='little')
bCMD_CHALLOK = int.to_bytes(CMD_CHALLOK, length=4, byteorder='little')
bCMD_PUB = int.to_bytes(CMD_PUB, length=4, byteorder='little')
bCMD_SUB = int.to_bytes(CMD_SUB, length=4, byteorder='little')
bCMD_UNSUB = int.to_bytes(CMD_UNSUB, length=4, byteorder='little')
bCMD_SENDPUBLIC = int.to_bytes(CMD_SENDPUBLIC, length=4, byteorder='little')
bCMD_PUBPUBLIC = int.to_bytes(CMD_PUBPUBLIC, length=4, byteorder='little')
bCMD_SENDPT = int.to_bytes(CMD_SENDPT, length=4, byteorder='little')
bCMD_FORWARDPT = int.to_bytes(CMD_FORWARDPT, length=4, byteorder='little')
bCMD_DATAPT = int.to_bytes(CMD_DATAPT, length=4, byteorder='little')
bCMD_SUBOK = int.to_bytes(CMD_SUBOK, length=4, byteorder='little')
