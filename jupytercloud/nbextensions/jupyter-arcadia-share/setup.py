# -*- coding: utf-8 -*-

import setuptools

setuptools.setup(
    name='jupyter_arcadia_share',
    packages=setuptools.find_packages(),
    description="Add Arcadia share button on the Jupyter pages",
    version='0.3',
    author='Vladimir Lipkin',
    author_email='lipkin@yandex-team.ru',
    project_urls={
        'ABC Service': 'https://abc.yandex-team.ru/services/jupyter-cloud/',
    },
    include_package_data=True,
    data_files=[
        ('share/jupyter/nbextensions/jupyter_arcadia_share', [
            'jupyter_arcadia_share/static/index.js',
            'jupyter_arcadia_share/static/share_form.js',
        ]),
        ('etc/jupyter/nbconfig/common.d', [
            'jupyter-config/nbconfig/common.d/jupyter_arcadia_share.json'
        ]),
    ],
    zip_safe=False,
)
