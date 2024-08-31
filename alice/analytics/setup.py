#!/usr/bin/env python
# encoding: utf-8

# Файл нужен для того чтобы можно было все utils доставлять на YT с помощью механизма DevelopPackage в nile

from setuptools import setup, find_packages


if __name__ == '__main__':
    setup(
        name='voice-analytics',
        version='1.0.0',
        author='Voice Analytics',
        author_email='yoschi@yandex-team.ru',  # Рассылочки у нас нет...
        url='https://bb.yandex-team.ru/projects/VOICE/repos/analytics/browse',
        packages=find_packages('.', include=['utils*']),
        package_data={},
        include_package_data=True,
        zip_safe=False,
    )

