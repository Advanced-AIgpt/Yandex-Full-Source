# flake8: noqa
from __future__ import unicode_literals

from alice.vins.api_helper.standard_settings import *  # noqa
from vins_core.utils.data import find_vinsfile

CONNECTED_APPS = {
    'gc_skill': {
        'path': find_vinsfile('gc_skill'),
        'class': 'gc_skill.app.ExternalSkillApp',
        'ignore_mongo_errors': True,
    },
}
