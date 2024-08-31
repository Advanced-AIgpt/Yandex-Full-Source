# coding: utf-8

from ..core.events import listener


@listener('ybot.telegram.command')
def pong(ctx, event_name, value):
    text = value.message.text
    if text and text.startswith('/ping'):
        value.send_message(text='pong')
