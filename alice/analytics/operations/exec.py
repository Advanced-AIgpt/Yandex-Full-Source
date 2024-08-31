#!/usr/bin/env python
# encoding: utf-8
"""
Выполняет произвольный код, записанный в аргумент "code"
"""
from nirvana.job_context import context


if __name__ == '__main__':
    exec(context().get_parameters()['code'])
