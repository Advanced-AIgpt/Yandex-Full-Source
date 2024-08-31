from distutils.core import setup

setup(
        name='query-wizard-features-reader',
        version='0.1.7',
        author='@movb',
        author_email='movb@yandex-team.ru',
        url='https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/py_libs/query_wizard_features_reader',
        packages=['query_wizard_features_reader'],
        package_data={'query_wizard_features_reader': ['query_wizard_features_reader.so']}
)
