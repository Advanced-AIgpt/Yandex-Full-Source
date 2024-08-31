import re

from alice.megamind.protos.scenarios.directives_pb2 import TDirective
from alice.protos.extensions.extensions_pb2 import SpeechKitName


def test_all_speechkit_name_is_snake_case():
    for fd in TDirective.DESCRIPTOR.fields:
        if fd.name in {'EndpointId'}:
            continue

        opts = fd.message_type.GetOptions()
        name = opts.Extensions[SpeechKitName]
        if name in {'__callback__', '__none__'}:
            continue

        assert re.match(r'^[a-z]+([a-z0-9])*(_[a-z0-9]+)*$', name)
