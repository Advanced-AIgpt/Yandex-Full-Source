from distutils.core import setup

setup(
    name='vins-models-tf',
    version='0.5.2',
    author='@smirnovpavel',
    author_email='smirnovpavel@yandex-team.ru',
    url='https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/py_libs/vins_models_tf',
    packages=['vins_models_tf'],
    package_data={'vins_models_tf': ['vins_models_tf.so']}
)
