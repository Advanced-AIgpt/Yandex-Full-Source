# coding:utf-8
import attr
import json

import alice.gamma.sdk.api.card_pb2 as api


@attr.s(frozen=True)
class Header:
    text = attr.ib(type=str)

    def to_proto(self):
        return api.CardHeader(
            text=self.text,
        )


@attr.s(frozen=True)
class Button:
    text = attr.ib(type=str)
    url = attr.ib(type=str)
    payload = attr.ib(default=None)

    def to_proto(self):
        return api.CardButton(
            text=self.text,
            url=self.url,
            payload=json.dumps(self.payload).encode() if self.payload else None,
        )


@attr.s(frozen=True)
class Item:
    image_id = attr.ib(type=str)
    title = attr.ib(type=str)
    description = attr.ib(type=str)
    button = attr.ib(type=Button, default=None)

    def to_proto(self):
        return api.CardItem(
            imageId=self.image_id,
            title=self.title,
            description=self.description,
            button=self.button.to_proto() if self.button else None,
        )

    @staticmethod
    def items_to_proto(items):
        return [item.to_proto() for item in items]


@attr.s(frozen=True)
class Footer:
    text = attr.ib(type=str)
    button = attr.ib(type=Button, default=None)

    def to_proto(self):
        return api.CardFooter(
            text=self.text,
            button=self.button.to_proto() if self.button else None,
        )


@attr.s(frozen=True)
class Card:
    type = attr.ib(type=str)
    image_id = attr.ib(type=str)
    title = attr.ib(type=object)
    description = attr.ib(type=str)
    button = attr.ib(type=Button, default=None)
    header = attr.ib(type=Header, default=None)
    footer = attr.ib(type=Footer, default=None)
    items = attr.ib(type=list, default=list())

    def add_items(self, *items):
        self.items.extend(items)

    def to_proto(self):
        return api.Card(
            type=self.type,
            imageId=self.image_id,
            title=self.title,
            description=self.description,
            button=self.button.to_proto() if self.button else None,
            header=self.header.to_proto() if self.header else None,
            footer=self.footer.to_proto() if self.footer else None,
            items=Item.items_to_proto(self.items) if self.items else None,
        )
