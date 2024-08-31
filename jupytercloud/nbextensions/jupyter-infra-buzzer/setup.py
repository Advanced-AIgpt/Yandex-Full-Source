# -*- coding: utf-8 -*-

import setuptools

setuptools.setup(
    name='jupyter_infra_buzzer',
    packages=setuptools.find_packages(),
    description="Add InfraBuzzer on the Jupyter pages",
    version='0.1',
    author='Vladimir Lipkin',
    author_email='lipkin@yandex-team.ru',
    project_urls={
        'ABC Service': 'https://abc.yandex-team.ru/services/jupyter-cloud/',
    },
    include_package_data=True,
    data_files=[
        ('share/jupyter/nbextensions/jupyter_infra_buzzer', [
            'jupyter_infra_buzzer/static/index.js',
        ]),
        ('etc/jupyter/nbconfig/common.d', [
            'jupyter-config/nbconfig/common.d/jupyter_infra_buzzer.json'
        ]),
    ],
    zip_safe=False,
)
