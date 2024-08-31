# coding:utf-8
import attr
import json

import alice.gamma.sdk.api.api_pb2 as api


@attr.s(frozen=True)
class Button:
    title = attr.ib(type=str)
    url = attr.ib(type=str, default=None)
    payload = attr.ib(default=None)
    hide = attr.ib(type=bool, default=False)

    def to_proto(self):
        return api.Button(
            title=self.title,
            url=self.url,
            payload=json.dumps(self.payload, ensure_ascii=False).encode(encoding='utf-8') if self.payload else None,
            hide=self.hide,
        )

    @staticmethod
    def buttons_to_proto(buttons):
        return [button.to_proto() for button in buttons]
