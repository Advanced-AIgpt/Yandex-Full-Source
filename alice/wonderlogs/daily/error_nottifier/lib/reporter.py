from alice.wonderlogs.daily.error_nottifier.lib.juggler import Status


def report(rows, thresholds):
    checks = []
    worst_status = Status.OK
    for row in rows:
        process = row['process']
        reason = row['reason']
        count = row['count']
        share = row['share']
        cur_status = Status.OK
        for status, threshold_status in ((Status.WARN, thresholds[process][reason]['Warn']),
                                         (Status.CRIT, thresholds[process][reason]['Crit'])):
            if 'Absolute' in threshold_status:
                if threshold_status['Absolute'] <= count:
                    cur_status = status
            elif 'Share' in threshold_status:
                if threshold_status['Share'] <= share:
                    cur_status = status
        checks.append((process, reason, str(cur_status)))
        if cur_status.worse(worst_status):
            worst_status = cur_status

    return str(worst_status), checks
