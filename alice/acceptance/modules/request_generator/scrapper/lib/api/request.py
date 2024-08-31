# coding: utf-8

import enum

import attr


@enum.unique
class Command(enum.Enum):
    START = b'start'
    CONTINUE = b'continue'
    START_SESSION = b'start_session'
    WAITING = b'waiting'


@attr.s
class Request:
    command = attr.ib()
    payload = attr.ib()

    @command.validator
    def command_validate(self, attribute, value):
        if not isinstance(value, Command):
            raise ValueError('Invalid request command value type')
