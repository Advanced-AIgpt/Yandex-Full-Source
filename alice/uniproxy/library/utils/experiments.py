from collections.abc import Mapping


def safe_experiments_vins_format(experiments, warn_func):
    # VOICESERV-1411: MUST return {string: string|None}
    if isinstance(experiments, (list, tuple)):  # change type to dict(list_item: '1')
        # ensure has list of strings
        orig_len = len(experiments)
        experiments = [it for it in experiments if isinstance(it, str)]
        if orig_len != len(experiments):
            warn_func("Filter {} incorrect experiments flags".format(orig_len - len(experiments)))
        experiments = dict.fromkeys(experiments, '1')
    elif not isinstance(experiments, Mapping):
        warn_func("Unsupported experiments format: {}".format(str(type(experiments))))
        experiments = {}
    return experiments


def weak_update_experiments(experiments, new_experiments):
    # https://st.yandex-team.ru/VOICESERV-1949#5cecff9359c4fd0021a30ffd
    # ^^^ we MUST NOT rewrite exist flags
    for k, v in new_experiments.items():
        if k not in experiments:
            experiments[k] = v


def _get_experiments_from_payload(payload):
    return payload.get("request", {}).get("experiments", {})


def _check_experiment(experiments, name):
    return bool(experiments) and bool(experiments.get(name, False))


# Helper function for vinsadapter, vins processor and maybe something else
def conducting_experiment(name, payload, uaas_flags=None):
    experiments = _get_experiments_from_payload(payload)

    # VOICESERV-3145
    disregard_uaas = _check_experiment(experiments, "disregard_uaas")
    if not disregard_uaas:
        if uaas_flags and name in uaas_flags:
            return True

    # VOICESERV-3145 "disregard_uaas" for one event
    if disregard_uaas and "experiments_without_uaas_flags" in payload.get("request", {}):
        experiments = payload.get("request", {}).get("experiments_without_uaas_flags", {})

    return _check_experiment(experiments, name)


# return experiment value or None, if experiment not found
def experiment_value(name, payload, uaas_flags=None):
    if uaas_flags and isinstance(uaas_flags, Mapping) and name in uaas_flags:
        return uaas_flags[name]

    experiments = _get_experiments_from_payload(payload)
    if not experiments or not isinstance(experiments, Mapping):
        return None

    return experiments.get(name)


def _iterate_over_experiments(payload, uaas_flags=None):
    if uaas_flags and isinstance(uaas_flags, Mapping):
        for key, value in uaas_flags.items():
            yield (key, value)

    experiments = _get_experiments_from_payload(payload)
    if experiments:
        if isinstance(experiments, Mapping):
            for key, value in experiments.items():
                yield (key, value)
        elif isinstance(experiments, list):
            for key in experiments:
                yield (key, "1")


# return value of experiment "exp_name=value"
def mm_experiment_value(name, payload, uaas_flags=None):
    _name = name + '='
    experiments = _iterate_over_experiments(payload, uaas_flags)

    for (e, _) in experiments:
        if e.startswith(_name):
            return e[len(_name):]
    return None
