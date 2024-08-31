#!/usr/bin/env python
# encoding: utf-8


class ValidationError(Exception):
    pass


class DuplicatedSubTree(ValidationError):
    pass
