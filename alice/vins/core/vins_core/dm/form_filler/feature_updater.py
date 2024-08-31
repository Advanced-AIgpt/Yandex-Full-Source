import attr


@attr.s
class FeatureUpdaterResult(object):
    sample_features = attr.ib()
    form_candidates = attr.ib()
    data = attr.ib(default={})


class FeatureUpdater(object):
    NAME = None

    def update_features(self, app, sample_features, session, req_info, form_candidates):
        raise NotImplementedError


_feature_updater_factories = {}


def create_feature_updater(name, **kwargs):
    if name is None:
        return None
    if name not in _feature_updater_factories:
        raise ValueError('Unknown feature updater type: %s' % name)
    return _feature_updater_factories[name](**kwargs)


def register_feature_updater(cls):
    _feature_updater_factories[cls.NAME] = cls
