from alice.uniproxy.library.events import ExtraData


def test_event_construction():
    extra = ExtraData.try_parse({
        "extra": {
            "header": {
                "namespace": "Foo",
                "name": "Bar",
                "initMessageId": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee",
                "extType": "Baz"
            },
            "payload": {}
        }
    })

    assert extra.namespace == "foo"
    assert extra.name == "bar"
    assert extra.init_message_id == "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"
    assert extra.ext == "baz"
    assert extra.proc_id == "foo.bar-aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"
