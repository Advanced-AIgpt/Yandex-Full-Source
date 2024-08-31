# coding: utf-8

import sys


CONTENT_LENGTH_BYTES_SIZE = 10
CONTENT_LENGTH_FORMAT = '{:<' + str(CONTENT_LENGTH_BYTES_SIZE) + 'd}'
CONTENT_MAX_LENGTH = int('9' * CONTENT_LENGTH_BYTES_SIZE)


class IOException(Exception):
    pass


def write(message, fobject=sys.stdout.buffer):
    if isinstance(message, str):
        message = message.encode('utf-8')
    if len(message) > CONTENT_MAX_LENGTH:
        raise IOException('too long message for binary api')

    content_length_str = CONTENT_LENGTH_FORMAT.format(len(message))
    fobject.write(content_length_str.encode('utf-8'))
    fobject.write(message)
    fobject.flush()


def read(fobject=sys.stdin):
    for line in fobject:
        yield line
