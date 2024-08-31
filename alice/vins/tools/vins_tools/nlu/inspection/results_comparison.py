import json
import logging
import os
import sys


logger = logging.getLogger(__name__)


def _report_and_exit(message):
    logger.error(message)
    sys.exit(1)


def _check_metric(parent_name, metric_name, reference_values, new_values, delta):
    failures = []
    if metric_name not in reference_values:
        failures.append('Failed to find {} score in {} section in reference results file'.format(
            metric_name, parent_name))

    if metric_name not in new_values:
        failures.append('Failed to find {} score in {} section in new results file'.format(
            metric_name, parent_name))

    if not reference_values[metric_name] - new_values[metric_name] < delta:
        failures.append("Failed to check {} {}: reference value {}, new value {}".format(
            parent_name, metric_name, reference_values[metric_name], new_values[metric_name]))

    return failures


def report_and_exit_error_stats(errors_stats):
    if os.path.exists(errors_stats):
        with open(errors_stats) as f:
            _report_and_exit(f.read())
    else:
        logger.info('Everything is OK')


def compare_reports(
    report_reference, report_new, metrics, delta, error_stats=None
):
    """
    :param report_reference: reference results
    :param report_new: new report to check
    :param metrics: additional list of metrics to check
    :param delta: error level for delta for metrics decreasing
    :param error_stats: path to file for appending error stats, otherwise failing on error
    """
    logger.info('Compare {} with {}'.format(report_reference, report_new))

    if isinstance(report_reference, basestring):
        with open(report_reference, 'rt') as fd:
            data_ref = json.load(fd)
    else:
        data_ref = report_reference

    if isinstance(report_new, basestring):
        with open(report_new, 'rt') as fd:
            data_new = json.load(fd)
    else:
        data_new = report_new

    failures = list()

    failures.extend(_check_metric('overall', 'accuracy', data_ref, data_new, delta))
    failures.extend(_check_metric('overall', 'weighted_f_score', data_ref, data_new, delta))

    if len(metrics) > 0:
        if 'report' not in data_ref:
            _report_and_exit('Failed to find "report" section in reference results file')
        if 'report' not in data_new:
            _report_and_exit('Failed to find "report" section in new results file')

        for metric in metrics:
            if metric not in data_ref['report']:
                _report_and_exit('Failed to find {} in reference results file'.format(metric))
            if metric not in data_new['report']:
                _report_and_exit('Failed to find {} in new results file'.format(metric))
            metric_ref = data_ref['report'][metric]
            metric_new = data_new['report'][metric]

            failures.extend(_check_metric(metric, 'f1_measure', metric_ref, metric_new, delta))

    if len(failures) > 0:
        report = '\n[reference]: %s [new]: %s\n' % (report_reference, report_new)
        report += '\n'.join(failures)
        report += '\n================='
        if not error_stats:
            _report_and_exit(report)
        else:
            with open(error_stats, mode='a') as fout:
                fout.write(report)


def do_compare_p_values(report, metrics, min_support, critical_value, error_stats):
    with open(report, 'rt') as fd:
        data = json.load(fd)
    # check that the metrics exist in the report
    sample_value = data['report'].values()[0]
    existing_metrics = []
    issues = []
    for metric in metrics:
        if metric not in sample_value:
            issue = 'Metric {} was not found in the report {}. Probably, baseline was not found.'.format(metric, report)
            logger.warn(issue)
            issues.append(issue)
        else:
            existing_metrics.append(metric)

    for intent_name in sorted(data['report'].keys()):
        intent_results = data['report'][intent_name]
        # check only the intents that are large enough
        if 'support' in intent_results and intent_results['support'] >= min_support:
            for m in existing_metrics:
                if intent_results[m] <= critical_value:
                    issue = 'Metric {:15} for intent {:50} has significanlty ({:3.3f}) deteriorated'.format(
                        '"{}"'.format(m), '"{}"'.format(intent_name), intent_results[m]
                    )
                    if m.endswith('_pv') and m[:-3] in intent_results:
                        issue += ': {:3.3f}'.format(intent_results[m[:-3]])
                        if m[:-3] + '_base' in intent_results:
                            issue += ' vs {:3.3f}'.format(intent_results[m[:-3] + '_base'])
                        if 'support' in intent_results:
                            issue += ', {:d} obs'.format(int(intent_results['support']))
                    issue += '.'
                    issues.append(issue)
    if len(issues) > 0:
        result = '\n[report file]: %s\n' % (report)
        result += '\n'.join(issues)
        result += '\n================='
        if not error_stats:
            _report_and_exit(result)
        else:
            with open(error_stats, mode='a') as fout:
                fout.write(result)
