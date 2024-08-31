# coding: utf-8

import json
import click

from vins_core.utils.data import TarArchive, load_data_from_file

from vins_tools.nlu.ner.fst_custom import NluFstCustomMorphyConstructor


@click.command()
@click.argument('archive_path')
@click.option('--to-test-update', default=False, is_flag=True)
def main(archive_path, to_test_update=False):
    custom_currency = NluFstCustomMorphyConstructor(
        fst_name='custom_currency',
        data=load_data_from_file('vins_core/test/test_data/custom_currency.json')
    ).compile()

    test_entity_data = {
        u'test': [u'test', u'тест'],
    }

    if to_test_update:
        test_entity_data.update(
            {
                u'not_test': [u'not test'],
                u'lol': [u'lol', u'ololo-trololo', u'лол'],
            }
        )

    test_entity = NluFstCustomMorphyConstructor(
        fst_name='test_entity',
        data=test_entity_data
    ).compile()

    with TarArchive(archive_path, mode=b'w:gz') as arch:
        custom_currency.save_to_archive(arch)
        test_entity.save_to_archive(arch)


if __name__ == '__main__':
    main()
