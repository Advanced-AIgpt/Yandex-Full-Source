import argparse
import alice.hollywood.library.scenarios.alice_show.proto.fast_data_pb2 as alice_show
import alice.hollywood.library.scenarios.blueprints.proto.blueprints_fastdata_pb2 as blueprints
import alice.hollywood.library.scenarios.general_conversation.proto.general_conversation_pb2 as general_conversation
import alice.hollywood.library.scenarios.hardcoded_response.proto.hardcoded_response_pb2 as hardcoded_response
import alice.hollywood.library.scenarios.how_to_spell.proto.fast_data_pb2 as how_to_spell
import alice.hollywood.library.scenarios.market.common.proto.fast_data_pb2 as market
import alice.hollywood.library.scenarios.music.proto.fast_data_pb2 as music
import alice.hollywood.library.scenarios.news.proto.news_fast_data_pb2 as news
import alice.hollywood.library.scenarios.notifications.proto.notifications_pb2 as notifications
import alice.hollywood.library.scenarios.random_number.proto.random_number_fastdata_pb2 as random_number
import alice.hollywood.library.scenarios.sssss.proto.fast_data_pb2 as sssss
import alice.megamind.protos.common.events_pb2 as events
import alice.megamind.protos.scenarios.analytics_info_pb2 as analytics_info
import alice.megamind.protos.scenarios.request_meta_pb2 as meta
import alice.megamind.protos.scenarios.request_pb2 as req
import alice.megamind.protos.scenarios.response_pb2 as res
import google.protobuf.text_format as text
import proxied_alice_meta_info_pb2 as alice_meta_info
import sys


PROTO_CLASSES_TUPLE = (
    req.TScenarioRunRequest,
    req.TScenarioApplyRequest,
    res.TScenarioRunResponse,
    res.TScenarioApplyResponse,
    res.TScenarioCommitResponse,
    general_conversation.TGeneralConversationFastDataProto,
    general_conversation.TGeneralConversationProactivityFastDataProto,
    hardcoded_response.THardcodedResponseFastDataProto,
    how_to_spell.THowToSpellFastDataProto,
    market.TMarketFastData,
    notifications.TNotificationsFastDataProto,
    sssss.TSSSSFastDataProto,
    music.TMusicFastDataProto,
    music.TMusicShotsFastDataProto,
    music.TStationPromoFastDataProto,
    meta.TRequestMeta,
    news.TNewsFastDataProto,
    random_number.TThrowDiceFastDataProto,
    events.TEvent,
    alice_meta_info.TAliceMetaInfo,
    analytics_info.TAnalyticsInfo,
    blueprints.TBlueprintsFastDataProto,
    alice_show.TAliceShowFastDataProto,
)

PROTO_CLASSES = {
    cls.__name__: cls
    for cls in PROTO_CLASSES_TUPLE
}


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('in_path', nargs='?', default='/dev/stdin')
    parser.add_argument('out_path', nargs='?', default='/dev/stdout')
    parser.add_argument(
        '--proto',
        metavar='CLASS',
        required=True,
        choices=PROTO_CLASSES.keys(),
        help=f'Protobuf to convert, choices: {", ".join(PROTO_CLASSES.keys())}',
    )
    parser.add_argument('--to-text', action='store_true', dest='bin_to_text')
    parser.add_argument('--to-binary', action='store_false', dest='bin_to_text')

    return parser.parse_args()


def main():

    if len(PROTO_CLASSES) != len(PROTO_CLASSES_TUPLE):
        proto_classes = set(PROTO_CLASSES.keys())
        duplicate_protos = []
        for proto in PROTO_CLASSES_TUPLE:
            if proto.__name__ not in proto_classes:
                duplicate_protos.append(proto.__name__)
            else:
                proto_classes.remove(proto.__name__)
        print("Name conflict in proto classes:", ", ".join(duplicate_protos))
        exit(1)
    args = parse_args()

    cls = PROTO_CLASSES[args.proto]

    if args.bin_to_text:
        with open(args.in_path, 'rb') as fobj:
            in_bytes = fobj.read()
        message = cls()
        message.ParseFromString(in_bytes)
        with open(args.out_path, 'w') as fobj:
            fobj.write(text.MessageToString(message, as_utf8=True))
    else:
        with open(args.in_path, 'r') as fobj:
            in_str = fobj.read()
        message = cls()
        text.Merge(in_str, message)
        with open(args.out_path, 'wb') as fobj:
            fobj.write(message.SerializeToString())


if __name__ == '__main__':
    sys.exit(main() or 0)
