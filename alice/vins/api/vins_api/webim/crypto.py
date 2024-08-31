# coding: utf-8

from __future__ import unicode_literals

import base64
import struct
import json
import os
import logging

from Crypto.Cipher import AES

logger = logging.getLogger(__name__)


class Decryptor(object):
    def __init__(self):
        try:
            keys_raw = os.environ.get("CRMBOT_FRONTEND_DECRYPTION_KEY")
            self.keys = {k: v.decode('hex') for k, v in json.loads(keys_raw).iteritems()}
        except ValueError:
            logger.error("Error loading frontend decryption keys")
            self.keys = {}

    @staticmethod
    def _unpad(msg):
        return msg[:-ord(msg[len(msg)-1:])]

    def decrypt(self, msg):
        msg_decoded = base64.b64decode(msg)
        version = struct.unpack('>i', msg_decoded[0:4])
        iv = msg_decoded[4:20]
        message = msg_decoded[20:]
        try:
            key = self.keys['key_v{}'.format(version[0])]
        except KeyError:
            logger.error("Decryption key version {} not found!".format(version[0]))
            return None
        decrptr = AES.new(key, AES.MODE_CBC, iv)
        try:
            decrypted = decrptr.decrypt(message)
            unpaded = Decryptor._unpad(decrypted)
            return json.loads(unpaded)
        except ValueError:
            logger.error("Failed to decrypt message: {}".format(msg))
            return None
