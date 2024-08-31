from verstehen.config import set_defaults_to_indexes_configs


class IndexRegistry:
    name_to_class = dict()
    default_config_map = dict()

    @staticmethod
    def create_index(index_config, texts, payload=None, indexes_map=None):
        index_config = dict(index_config)
        set_defaults_to_indexes_configs(index_config, IndexRegistry.default_config_map)

        index_type = index_config['index_type']

        if index_type not in IndexRegistry.name_to_class:
            raise ValueError(
                'Index type `{}` is not in registry. Available index types: {}'.format(
                    index_type, list(IndexRegistry.name_to_class.keys())
                )
            )

        index_cls = IndexRegistry.name_to_class[index_type]
        return index_cls.from_config(index_config, texts, payload=payload, indexes_map=indexes_map)

    @staticmethod
    def create_indexes_by_skill_id(index_config, texts, skill_to_idxs_map, payload=None, indexes_map=None):
        index_config = dict(index_config)
        set_defaults_to_indexes_configs(index_config, IndexRegistry.default_config_map)

        index_type = index_config['index_type']

        if index_type not in IndexRegistry.name_to_class:
            raise ValueError(
                'Index type `{}` is not in registry. Available index types: {}'.format(
                    index_type, list(IndexRegistry.name_to_class.keys())
                )
            )

        index_cls = IndexRegistry.name_to_class[index_type]
        return index_cls.from_config_by_skill_id(
            index_config,
            texts,
            skill_to_idxs_map,
            payload=payload,
            indexes_map=indexes_map
        )


def registered_index(cls):
    index_type = cls.DEFAULT_CONFIG['index_type']
    IndexRegistry.name_to_class[index_type] = cls
    IndexRegistry.default_config_map[index_type] = cls.DEFAULT_CONFIG
    return cls
