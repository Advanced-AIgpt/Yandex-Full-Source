# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.common.sample import Sample
from vins_core.ner.fst_custom import NluFstCustom
from vins_core.utils.data import TarArchive, load_data_from_file

from vins_tools.nlu.ner.fst_custom import NluFstCustomMorphyConstructor


@pytest.fixture(scope='function')
def tar_archive(mocker, tmpdir):
    tar_file = tmpdir.join('archive.tar.gz').strpath
    mocker.patch('vins_core.utils.data.vins_temp_dir', lambda: tmpdir.strpath)
    arch = TarArchive(tar_file, mode=b'w:gz')
    yield arch


def test_save_fst(tar_archive):
    name = 'custom_currency'
    parser = NluFstCustomMorphyConstructor(
        fst_name=name,
        data=load_data_from_file('vins_core/test/test_data/custom_currency.json')
    ).compile()

    with tar_archive as arch:
        parser.save_to_archive(arch)

    with TarArchive(tar_archive.path) as arch:
        fst = NluFstCustom.load_from_archive(name, arch)

    sample = Sample.from_string('сколько стоит доллар')
    res = fst.parse(sample)

    assert [(r.type, r.value) for r in res] == [
        ('', 'сколько'),
        ('', 'стоит'),
        ('CUSTOM_CURRENCY', 'доллар'),
    ]
