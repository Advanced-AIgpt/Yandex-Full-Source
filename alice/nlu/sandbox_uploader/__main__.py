from __future__ import print_function

import nirvana.job_context as nv
import yalibrary.upload.uploader as uploader

import logging
import os


def main(dir_to_upload, resource_type, owner, ttl, token):
    resource_id = uploader.do(
        [dir_to_upload],
        resource_type=resource_type,
        resource_description='Regular {} deployment'.format(dir_to_upload),
        resource_owner=owner,
        ttl=ttl,
        sandbox_token=token,
    )
    print('Uploaded resource with ID {}'.format(resource_id))


if __name__ == "__main__":
    logging.basicConfig(format='%(levelname)-8s %(asctime)-27s %(message)s', level=logging.INFO)

    ctx = nv.context()
    unpacked_dir = ctx.get_inputs().get_item('input.tar').unpacked_dir()
    dir_to_upload = ctx.get_parameters()['dir-to-upload']
    os.rename(unpacked_dir, dir_to_upload)

    resource_type = ctx.get_parameters()['resource-type']
    owner = ctx.get_parameters()['resource_owner']
    ttl = ctx.get_parameters()['resource_ttl']
    token = ctx.get_parameters()['sandbox_token']

    main(dir_to_upload, resource_type, owner, ttl, token)
