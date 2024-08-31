import vh


def update_config(config, update_by):
    for dct in [config, config['_data']]:
        dct['_updated'] = set()
        for param in dct:
            if update_by.get(param):
                dct[param] = update_by[param]
                dct['_updated'].add(param)


def load_nirvana_data(config):
    data = {k: vh.data(id=v) for k, v in config['_data'].items() if v and not k.startswith('_')}
    updated_dssm_models = {k: data[k] for k in config['_data']['_updated'] if k.startswith(('base_model', 'factor_model'))}
    return data, updated_dssm_models
