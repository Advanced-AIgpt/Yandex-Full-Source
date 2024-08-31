import json

import click
from google.protobuf.descriptor_pb2 import FileDescriptorSet

from alice.wonderlogs.proto_tests.lib.proto_checker import compare_file_descriptor_sets


@click.command()
@click.option('--old-image')
@click.option('--new-image')
def main(old_image, new_image):
    old_file_descriptor_set = FileDescriptorSet()
    with open(old_image, 'rb') as f:
        old_file_descriptor_set.ParseFromString(f.read())

    new_file_descriptor_set = FileDescriptorSet()
    with open(new_image, 'rb') as f:
        new_file_descriptor_set.ParseFromString(f.read())

    passed, message = compare_file_descriptor_sets(old_file_descriptor_set, new_file_descriptor_set)

    print(json.dumps({'passed': passed, 'message': message}))


if __name__ == '__main__':
    main()
