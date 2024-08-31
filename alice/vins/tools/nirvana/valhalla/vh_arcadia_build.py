# coding: utf-8

import logging

from alice.vins.tools.nirvana.valhalla import op
from alice.vins.tools.nirvana.valhalla.global_options import vins_global_options

from library.python.svn_version import svn_revision

logging.basicConfig()
logger = logging.getLogger(__name__)


def arcadia_revision():
    return svn_revision()


def build_arcadia_binary(target, artifact, revision=None, patch=None, cuda=False):
    opts = {
        'arcadia_revision': revision,
        'targets': target,
        'arts': artifact,
        'owner': vins_global_options.sandbox_resources_owner,
        'sandbox_oauth_token': vins_global_options.sandbox_token,
        'ya_yt_token_vault_owner': vins_global_options.sandbox_vault_owner_yt_token,
        'ya_yt_token_vault_name': vins_global_options.sandbox_vault_name_yt_token,
        'aapi_fallback': True
    }
    if cuda:
        opts['definition_flags'] = (
            '-DTENSORFLOW_WITH_CUDA -DCUDA_VERSION=9.2 -DCUDA_NVCC_FLAGS="'
            '-gencode arch=compute_35,code=sm_35 -gencode arch=compute_50,code=compute_50 '
            '-gencode arch=compute_52,code=sm_52 -gencode arch=compute_60,code=compute_60 '
            '-gencode arch=compute_61,code=compute_61 -gencode arch=compute_61,code=sm_61 '
            '-gencode arch=compute_70,code=sm_70 -gencode arch=compute_70,code=compute_70"'
        )
    if patch:
        opts['arcadia_patch'] = patch
    return op.BUILD_ARC_OP(_options=opts)


def build_ya_package(arc_path, revision=None, patch=None):
    opts = {
        'arcadia_revision': revision,
        'packages': arc_path,
        'use_new_format': True,
        'owner': vins_global_options.sandbox_resources_owner,
        'sandbox_oauth_token': vins_global_options.sandbox_token,
    }
    if patch:
        opts['arcadia_patch'] = patch
    return op.YA_PACKAGE_OP(_options=opts)
