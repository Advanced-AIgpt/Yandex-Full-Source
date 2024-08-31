import argparse
import json
from collections import OrderedDict

from pymongo import MongoClient

from library.python.vault_client.instances import Production as VaultClient
from alice.hollywood.library.scenarios.draw_picture.resources.proto.resources_pb2 import TDrawPictureResourcesProto


def fetch_images(db_url, generations):
    client = MongoClient(db_url)
    cur = client.lvlasenkov.ganart.find(
        {
            'tag': {'$in': generations}
        },
        {
            'avatars.imagename': 1,
            'avatars.group-id': 1,
            'i2t': 1,
            '_id': 0
        },
    )
    print('fetching database...')
    images = OrderedDict()
    for i, doc in enumerate(cur):
        url = 'http://avatars.mds.yandex.net/get-milab/{}/{}'.format(
            doc['avatars']['group-id'],
            doc['avatars']['imagename'],
        )
        images[url] = (i, doc['i2t'])
    return images


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Upload GanArt databsae to sandbox.')
    parser.add_argument('--db', help='database URI or vault secret/version', type=str, default='sec-01e7mng7f6nnjbhhjpnp4ne9wn')
    parser.add_argument('-o', '--output', help='output file name', type=str, default='output.pb')
    parser.add_argument('tags', nargs='*', help='generation tags to fetch', type=str, default=['gen5_1', 'gen5_2', 'gen6_1'])
    parser.add_argument('-c', '--config', type=str, default='config.json')
    args = parser.parse_args()

    if args.db.startswith('mongo'):
        db_url = args.db
    else:
        db_url = VaultClient(decode_files=True).get_version(args.db)['value']['MONGODB_URI']

    with open(args.config, 'r') as f:
        config = json.load(f)
    images = fetch_images(db_url, args.tags)
    buckets = {}

    print('writing proto...')
    message = TDrawPictureResourcesProto()
    for i, (name, comments) in enumerate(config['buckets'].items()):
        buckets[name] = i
        bucket_msg = message.CommentBuckets.add()
        for comment in comments:
            comment_msg = bucket_msg.Comments.add()
            comment_msg.Comment = comment['comment']
            assert comment_msg.Comment
            tts = comment['tts']
            if tts is not None and tts != comment_msg.Comment:
                comment_msg.TTS = tts

    message.GenericBucketID = buckets[config['generic_bucket']]

    for i, (url, (j, features)) in enumerate(images.items()):
        assert i == j
        image_msg = message.Images.add()
        image_msg.URL = url
        image_msg.Features.extend(features)

    for substitute in config['substitutions']:
        substitute_msg = message.Substitutes.add()
        substitute_msg.CommentBucketID = buckets[substitute['bucket']]
        substitute_msg.Requests.extend(substitute['requests'])
        substitute_msg.ImageIDs.extend(images[url][0] for url in substitute['images'])

    message.Suggests.extend(config['suggests'])
    message.NLUHints.extend(config['nlu_hints'])

    with open(args.output, 'wb') as f:
        f.write(message.SerializeToString())
