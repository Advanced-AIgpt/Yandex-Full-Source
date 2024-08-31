from setuptools import setup, find_packages

setup(
    name='yandex-normalizer-general',
    version='0.1.1',
    description='voice normalizer scripts',
    long_description='voice normalizer scripts',
    author='SpeechKit Team',
    author_email='voice@yandex-team.ru',
    packages=find_packages(),
    include_package_data=True,
    package_data={'': ['*.txt', '*/*.txt', '*/*/*.txt', '*/*/*/*.txt', '*/*/*/*/*.txt']},
    install_requires=['yandex-pyfst'],
)
