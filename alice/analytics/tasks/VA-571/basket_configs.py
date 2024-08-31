# -*-coding: utf8 -*-

import sys
import json
import logging


DEFAULTS = {
    'new_format': True,
    'parse_iot': False,
    'only_smart_speakers': False
}


def _load_basket_configs():
    try:
        from library.python import resource
        return json.loads(resource.find('basket_configs.json'))
    except Exception as exc:
        logging.warning('Unable to load resource "basket_configs.json" [%s]', exc)
        return None


BASKET_CONFIGS = _load_basket_configs()
BASKET_CONFIGS_SET = set([x['alias'] for x in BASKET_CONFIGS]) if BASKET_CONFIGS else None


def override_params(config, override_basket_params):
    if isinstance(override_basket_params, str if sys.version_info[0] >= 3 else basestring):  # noqa
        override_basket_params = json.loads(override_basket_params)

    if override_basket_params:
        for override_basket_params_item in override_basket_params:
            # перезаписать параметры можно только по алиасу корзины, и mr_path перезаписывать нельзя
            if 'alias' not in override_basket_params_item:
                raise ValueError('override_basket_params should contain "alias" key: {}'.format(
                    json.dumps(override_basket_params_item)
                ))
            if 'mr_path' in override_basket_params_item:
                raise ValueError('override_basket_params should not contain "mr_path" key: {}'.format(
                    json.dumps(override_basket_params_item)
                ))

            if config['alias'] == override_basket_params_item['alias']:
                config.update(override_basket_params_item)


def get_basket_config(alias=None, mr_path=None, override_basket_params=None, base_config=None):
    all_configs = BASKET_CONFIGS
    if base_config:
        try:
            all_configs = json.loads(base_config)
        except Exception as exc:
            logging.error('Unable to parse base_config: %s', exc)
            raise
    assert all_configs, "Invalid all_configs value: %r" % all_configs

    if alias is None and mr_path is None:
        raise ValueError('Either alias or mr_path should be specified')

    for basket_config in all_configs:
        if (alias and basket_config['alias'] == alias) or \
                (mr_path and basket_config['mr_path'] in (mr_path, '//' + mr_path)):
            config = basket_config.copy()
            override_params(config, override_basket_params)
            return config

    raise ValueError('Basket {} {} is not registered. Please add it to BASKET_CONFIGS.'.format(
        'alias' if alias is not None else 'path',
        alias if alias is not None else mr_path
    ))


def get_basket_param(param, basket_alias=None, basket_path=None, override_basket_params=None, base_config=None):
    return get_basket_config(
        alias=basket_alias,
        mr_path=basket_path,
        override_basket_params=override_basket_params,
        base_config=base_config,
    ).get(param, DEFAULTS.get(param))


def mr_path_to_alias(mr_path, override_basket_params=None, base_config=None):
    return get_basket_param('alias', basket_path=mr_path, override_basket_params=override_basket_params,
                            base_config=base_config)


def alias_to_mr_path(alias, override_basket_params=None, base_config=None):
    return get_basket_param('mr_path', basket_alias=alias, override_basket_params=override_basket_params,
                            base_config=base_config)


def get_baskets_paths(input_basket=None, param_baskets=None, override_basket_params=None, base_config=None):
    if not input_basket and not param_baskets:
        raise ValueError('Either input_basket or param_baskets should be specified')

    if input_basket and input_basket != 'EMPTY' and input_basket != '//EMPTY':
        return [input_basket]
    else:
        return [alias_to_mr_path(basket, override_basket_params, base_config) for basket in param_baskets.split(',')]
