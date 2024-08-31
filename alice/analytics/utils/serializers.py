#!/usr/bin/env python
# encoding: utf-8

from collections import OrderedDict

import yaml
# Чтобы yaml парсил словари с упорядоченными ключами, нужно этот модуль хотя бы раз выполнить.
# Например, импортировать из него этот же yaml

# === Хак для того чтобы не терять упорядоченность в словарях, сериализованных в yaml:
_mapping_tag = yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG


def dict_representer(dumper, data):
    return dumper.represent_dict(data.iteritems())


def dict_constructor(loader, node):
    return OrderedDict(loader.construct_pairs(node))


yaml.add_representer(OrderedDict, dict_representer)
yaml.add_constructor(_mapping_tag, dict_constructor)
# === End of hack
