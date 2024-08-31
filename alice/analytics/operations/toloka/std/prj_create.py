#!/usr/bin/env python
# encoding: utf-8
from __future__ import unicode_literals

from utils.nirvana.op_caller import call_as_operation
from utils.toloka.api import GeneralRequester, PoolRequester
from utils.toloka.std_pool import StdPool
from utils.toloka.std_prj import StdProject


# Используется в "Create Toloka Std Project"
# https://nirvana.yandex-team.ru/operation/b893ca46-9bff-4762-afd0-53c59a0a262e/subprocess


def create_or_update_prj(
        title, instructions,
        inp_spec, out_spec,
        label_duration, label_count,
        abc_service_id=None, is_sandbox=True,
        public_description='', private_description='',
        custom_markup=None, custom_script=None, custom_styles=None
):
    assert (title and instructions), 'title и instructions должны быть непустыми'
    assert (inp_spec and out_spec), 'inp_spec и out_spec должны быть непустыми'
    assert (label_duration and label_count), 'label_duration и label_count должны быть ненулевыми'
    assert (abc_service_id is not None), 'abc_service_id должен быть задан'
    gr = GeneralRequester(is_sandbox=is_sandbox)
    all_projects = gr.list_all_projects(status='ACTIVE')

    found_project_id = None
    for project_params in all_projects:
        if project_params['public_name'] == title:
            found_project_id = project_params['id']

    params = {
        'title': title,
        'instructions': instructions,
        'inp_spec': inp_spec,
        'out_spec': out_spec,
        'public_description': public_description,
        'private_description': private_description,
        'custom_markup': custom_markup,
        'custom_script': custom_script,
        'custom_styles': custom_styles,
        'metadata': {
            'abc_service_id': abc_service_id,
        }
    }

    # Project not found, creating new one
    if found_project_id is None:
        prj = StdProject(is_sandbox=is_sandbox)
        prj.create_prj(**params)
    else:
        prj = StdProject(prj_id=found_project_id, is_sandbox=is_sandbox)

        # closing pools before anu project adjustments not to have possibly corrupted pools
        for pool in prj.list_open_pools():
            PoolRequester(prj_id=found_project_id, pool_id=pool['id'], is_sandbox=is_sandbox).close_pool()

        prj.update_prj(**params)

    prj.get_or_create_accuracy_skill()

    pool = StdPool(is_sandbox=is_sandbox,
                   prj_id=prj.prj_id, skill_id=prj.skill_id,
                   label_duration=label_duration, label_count=label_count,
                   out_spec=out_spec)

    return {
        'prj_id': prj.prj_id,
        'skill_id': prj.skill_id,
        'page_size': pool.page_size,
        'random_accuracy': pool.random_accuracy,
        'overlap': pool.overlap,
        'priority': pool.priority,
        'is_sandbox': is_sandbox,
        'prj_url': prj.get_prj_url(),
        'skill_url': prj.get_skill_url(),
        'prj_name_and_instructions_hash': prj.get_prj_name_and_instructions_hash()
    }


if __name__ == '__main__':
    call_as_operation(create_or_update_prj, input_spec={
        'instructions': {'link_name': 'instructions', 'parser': 'text'},
        'inp_spec': {'link_name': 'inp_spec', 'parser': 'json'},
        'out_spec': {'link_name': 'out_spec', 'parser': 'json'},
    })
