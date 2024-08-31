# -*- coding: utf-8 -*-

import setuptools

setuptools.setup(
    name='jupyter_yndxbug',
    packages=setuptools.find_packages(),
    description="Add yndxbug on the Jupyter pages",
    version='0.1',
    author='Vladimir Lipkin',
    author_email='lipkin@yandex-team.ru',
    project_urls={
        'ABC Service': 'https://abc.yandex-team.ru/services/jupyter-cloud/',
    },
    include_package_data=True,
    data_files=[
        ('share/jupyter/nbextensions/jupyter_yndxbug', [
            'jupyter_yndxbug/static/index.js',
        ]),
        ('etc/jupyter/nbconfig/common.d', [
            'jupyter-config/nbconfig/common.d/jupyter_yndxbug.json'
        ]),
    ],
    zip_safe=False,
)
