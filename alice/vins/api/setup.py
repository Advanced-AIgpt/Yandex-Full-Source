from setuptools import setup, find_packages

version = '0.1.1'

setup(
    name='vins-api',
    version=version,
    description='Voice INterfaces form-filler-dm application',
    long_description='Voice INterfaces form-filler-dm application',
    author='Evgeny Volkov',
    author_email='emvolkov@yandex-team.ru',
    packages=find_packages(),
    include_package_data=True,
)
