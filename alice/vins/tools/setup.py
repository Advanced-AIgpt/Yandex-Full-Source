from setuptools import setup, find_packages

setup(
    name='vins-tools',
    version='0.1.1',
    description='Voice INterfaces tools',
    long_description='',
    author='Grigory Kostin',
    author_email='g-kostin@yandex-team.ru',
    packages=find_packages(include=('vins_tools/*',)),
    include_package_data=True,
)
