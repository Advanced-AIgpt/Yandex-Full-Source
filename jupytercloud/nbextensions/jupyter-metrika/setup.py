# -*- coding: utf-8 -*-

import setuptools

setuptools.setup(
    name='jupyter_metrika',
    packages=setuptools.find_packages(),
    description="Add Yandex.Metrika counter to the Jupyter pages",
    version='0.1',
    author='JupyterCloud Team',
    author_email='jupyter-cloud-dev@yandex-team.ru',
    project_urls={
        'ABC Service': 'https://abc.yandex-team.ru/services/jupyter-cloud/',
    },
    include_package_data=True,
    data_files=[
        ('share/jupyter/nbextensions/jupyter_metrika', [
            'jupyter_metrika/static/index.js',
        ]),
        ('etc/jupyter/nbconfig/common.d', [
            'jupyter-config/nbconfig/common.d/jupyter_metrika.json'
        ]),
    ],
    zip_safe=False,
)
