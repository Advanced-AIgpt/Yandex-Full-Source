from collections import defaultdict


def generate_thresholds(error, thresholds):
    # res[process][reason][status_type][absolute/share] = threshold
    res = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(int))))
    for threshold in thresholds:
        process = error.EProcess.Name(threshold.Process)
        reason = error.EReason.Name(threshold.Reason)
        if threshold.Thresholds.Warn.HasField('Absolute'):
            res[process][reason]['Warn']['Absolute'] = threshold.Thresholds.Warn.Absolute
        elif threshold.Thresholds.Warn.HasField('Share'):
            res[process][reason]['Warn']['Share'] = threshold.Thresholds.Warn.Share
        else:
            assert False, 'Unknown status of the Warn threshold'

        if threshold.Thresholds.Crit.HasField('Absolute'):
            res[process][reason]['Crit']['Absolute'] = threshold.Thresholds.Crit.Absolute
        elif threshold.Thresholds.Warn.HasField('Share'):
            res[process][reason]['Crit']['Share'] = threshold.Thresholds.Crit.Share
        else:
            assert False, 'Unknown status of the Crit threshold'
    return res
