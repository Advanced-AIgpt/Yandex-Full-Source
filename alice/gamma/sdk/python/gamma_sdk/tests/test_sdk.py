# coding: utf-8

import pytest

import alice.gamma.sdk.api.api_pb2 as api
import alice.gamma.sdk.api.card_pb2 as card_api
from google.protobuf import struct_pb2

from gamma_sdk.sdk import sdk, card, button


def test_button_to_proto():
    btn = button.Button(title='Hi', payload={'foo': 'test'}, url='ya.ru')
    proto_button = api.Button(title='Hi', payload=b'{"foo": "test"}', url='ya.ru')
    assert proto_button == btn.to_proto(), 'ProtoButtons should be equal'


def test_response_add_buttons():
    response = sdk.Response('foo', 'bar')
    btn = button.Button(title='Hi', payload={'foo': 'test'}, url='ya.ru')
    btn2 = button.Button(title='Hi2', payload={'foo2': 2}, url='yayaya.ru')
    response.add_buttons(btn, btn2)
    expected_response = sdk.Response('foo', 'bar', buttons=[btn, btn2])
    assert expected_response == response, 'Responses should be equal'


@pytest.mark.parametrize('proto, result', [
    (
        api.RequestBody(command='test', originalUtterance='test', type='test', payload=b'{"foo": "test"}'),
        sdk.Request(command='test', original_utterance='test', type='test', payload={'foo': 'test'})
    ),
    (
        api.RequestBody(
            command='foo bar',
            originalUtterance='foo bar',
            type='test',
            nlu=api.Nlu(
                tokens=['foo', 'bar'],
                entities=[
                    api.Entity(
                        start=1,
                        end=2,
                        protoValue=struct_pb2.Value(string_value='{"subvalue": "бар"}'.encode('utf-8')),
                        type='test'
                    )
                ],
            )
        ),
        sdk.Request(
            command='foo bar',
            original_utterance='foo bar',
            type='test',
            nlu=sdk.Nlu(
                tokens=['foo', 'bar'],
                entities={
                    'test': [
                        sdk.Entity(
                            type='test',
                            begin=1,
                            end=2,
                            value=struct_pb2.Value(string_value='{"subvalue": "бар"}'.encode('utf-8')),
                        )
                    ]
                }
            )
        )
    ),
])
def test_request_from_proto(proto, result):
    assert sdk.Request.from_proto(proto) == result, 'Requests should be equal'


def test_card_button_to_proto():
    button = card.Button(text='Hi', payload={'foo': 'test'}, url='ya.ru')
    proto_button = card_api.CardButton(text='Hi', payload=b'{"foo": "test"}', url='ya.ru')
    assert proto_button == button.to_proto(), 'ProtoButtons should be equal'


def test_card_header_to_proto():
    header = card.Header(text='Hi')
    proto_header = card_api.CardHeader(text='Hi')
    assert proto_header == header.to_proto(), 'ProtoHeaders should be equal'


def test_card_footer_to_proto():
    footer = card.Footer(text='Hi!', button=card.Button(text='Bye!', payload={'foo': 2}, url='8.8.8.8'))
    proto_footer = card_api.CardFooter(
        text='Hi!',
        button=card_api.CardButton(text='Bye!', payload=b'{"foo": 2}', url='8.8.8.8')
    )
    assert proto_footer == footer.to_proto(), 'ProtoFooters should be equal'


def test_card_item_to_proto():
    item = card.Item(title='Hi', image_id='some_id/', description='kitty',
                     button=card.Button(text='Hi', payload={'foo': 'test'}, url='ya.ru'))
    proto_item = card_api.CardItem(
        title='Hi',
        imageId='some_id/',
        description='kitty',
        button=card_api.CardButton(text='Hi', payload=b'{"foo": "test"}', url='ya.ru')
    )
    assert proto_item == item.to_proto(), 'ProtoItems should be equal'


def test_card_to_proto():
    card_item = card.Item(title='Hi', image_id='some_id/', description='kitty',
                          button=card.Button(text='Hi', payload={'foo': 'test'}, url='ya.ru'))
    proto_item = card_api.CardItem(
        title='Hi',
        imageId='some_id/',
        description='kitty',
        button=card_api.CardButton(text='Hi', payload=b'{"foo": "test"}', url='ya.ru')
    )
    c = card.Card(type='SomeType', title='Hi', description='description', image_id='some_id/',
                  header=card.Header(text='Hi'),
                  button=card.Button(text='Hi', payload={'foo': 'test'}, url='ya.ru'),
                  items=[card_item],
                  footer=card.Footer(
                      text='Hi!',
                      button=card.Button(text='Bye!', payload={'foo': 2}, url='8.8.8.8'))
                  )

    proto_card = card_api.Card(type='SomeType', title='Hi', description='description', imageId='some_id/',
                               header=card_api.CardHeader(text='Hi'),
                               button=card_api.CardButton(text='Hi', payload=b'{"foo": "test"}', url='ya.ru'),
                               items=[proto_item],
                               footer=card_api.CardFooter(
                                   text='Hi!',
                                   button=card_api.CardButton(text='Bye!', payload=b'{"foo": 2}', url='8.8.8.8'))
                               )

    assert proto_card == c.to_proto(), 'Cards should be equal'
