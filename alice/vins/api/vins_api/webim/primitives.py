# coding: utf-8
from __future__ import unicode_literals

import falcon
import attr
import zlib

from vins_core.utils.strings import smart_utf8


@attr.s()
class TextMessage(object):
    text = attr.ib()

    def to_json(self):
        return {
            'kind': 'operator',
            'text': self.text
        }


@attr.s()
class FileMessage(object):
    url = attr.ib()
    name = attr.ib()
    media_type = attr.ib()

    def to_json(self):
        return {
            'kind': 'file_operator',
            'data': attr.asdict(self)
        }


@attr.s()
class Button(object):
    text = attr.ib()
    id = attr.ib(default=None)

    def to_json(self):
        if self.id is None:
            self.id = str(zlib.crc32(smart_utf8(self.text)))[0:23]
        return attr.asdict(self)


class Keyboard(object):
    def __init__(self):
        self.kbd = []

    def add_button(self, button, continue_row=False):
        if continue_row:
            self.kbd[-1].append(button)
        else:
            self.kbd.append([button])

    def empty(self):
        return len(self.kbd) == 0

    def to_json(self):
        result = []
        for r in self.kbd:
            row = []
            for button in r:
                row.append(button.to_json())
            result.append(row)
        return {
            'kind': 'keyboard',
            'buttons': result
        }


@attr.s()
class OperatorRedirect(object):
    operator_id = attr.ib(type=int, converter=int)
    chat_id = attr.ib(type=int, default=0, converter=int)

    def to_json(self):
        return attr.asdict(self)


@attr.s()
class DepartmentRedirect(object):
    dep_key = attr.ib()
    chat_id = attr.ib(type=int, default=0, converter=int)
    allow_redirect_to_offline_dep = attr.ib(default=False)
    allow_redirect_to_invisible_dep = attr.ib(default=True)

    def to_json(self):
        return attr.asdict(
            self,
            filter=attr.filters.exclude(
                attr.fields(DepartmentRedirect).allow_redirect_to_offline_dep,
                attr.fields(DepartmentRedirect).allow_redirect_to_invisible_dep
            )
        )


@attr.s()
class OCRMDepartmentRedirect(object):
    category = attr.ib(default="default")

    def to_json(self):
        return attr.asdict(self)


@attr.s()
class CloseChat(object):
    chat_id = attr.ib(type=int, default=0, converter=int)

    def to_json(self):
        return attr.asdict(self)


@attr.s()
class UserInfo(object):
    chat_id = attr.ib()
    uri = attr.ib()
    req_id = attr.ib()
    req_info = attr.ib()
    session_data = attr.ib(default=attr.Factory(dict))


@attr.s()
class VinsResponse(object):
    messages = attr.ib(default=attr.Factory(list))
    keyboard = attr.ib(default=None)
    status_code = attr.ib(default=falcon.HTTP_200)
    intent = attr.ib(default=None)

    def to_json(self):
        return {
            "messages": [attr.asdict(message) for message in self.messages],
            "keyboard": self.keyboard.to_json() if self.keyboard is not None else None,
            "intent": self.intent,
            "status_code": self.status_code
        }
