from .spotter_features import SpotterFeatures
from .storage_cachalot import ActivationStorageCachalot, ActivationStorageCachalotSimple
from .storage_null import ActivationStorageNull


def make_activation_storage(two_steps_activation, smart_activation_demanded,
                            uid, device_id, spotter_features, cachalot_kwargs={}):
    if not smart_activation_demanded:
        return ActivationStorageNull(uid, device_id, spotter_features)
    elif two_steps_activation:
        return ActivationStorageCachalot(uid, device_id, spotter_features, **cachalot_kwargs)
    else:
        return ActivationStorageCachalotSimple(uid, device_id, spotter_features, **cachalot_kwargs)


__all__ = [
    make_activation_storage,
    SpotterFeatures
]
