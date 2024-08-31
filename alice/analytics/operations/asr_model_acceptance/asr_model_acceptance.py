#!/usr/bin/env python
# encoding: utf-8

from scipy.stats import wilcoxon
import nirvana.job_context as nv
from utils.nirvana.op_caller import call_as_operation
import io


def nirvana_any_exception_to_custom_error_message(f):
    def decorated(*args, **kwargs):
        try:
            return f(*args, **kwargs)
        except Exception as e:
            ctx = nv.context()
            with io.open(ctx.get_status().get_error_msg(),
                         mode='wt') as error_msg:
                error_msg.write(unicode(str(e)))
            raise
    return decorated


@nirvana_any_exception_to_custom_error_message
def main(
        prod_stats,
        test_stats,
        prod_error_utterances,
        test_error_utterances,
        min_pvalue=0.05,
        max_test_rtf=0.8,
        max_prod_rtf=0.8,
        min_test_rtf=0.1,
        min_prod_rtf=0.1,
        max_test_wer=20.0,
        max_prod_wer=20.0,
        min_prod_wer=1.0,
        min_test_wer=1.0,
        min_testset_size=500
):
    prod_stats = prod_stats[0]
    test_stats = test_stats[0]
    assert len(prod_error_utterances) == len(test_error_utterances), 'Inequal testset lengths'
    testset_len = prod_stats['utterances']
    assert len(prod_error_utterances) == testset_len, 'Inequal number of utterances in prod stats and prod errors input'
    assert test_stats['utterances'] == testset_len, 'Inequal number of utterances in prod stats and test_stats'

    assert testset_len >= min_testset_size, 'Testset size is bellow lower bound %s' % str(min_testset_size)

    message = []
    invalid_prod = False
    invalid_test = False
    ship_it = True
    message.append('Comparison for model %s' % prod_stats['model'])
    message.append('Test set: %s' % prod_stats.get('testset', 'UNSPECIFIED'))
    message.append('Test set size: %i' % testset_len)

    prod_rtf = prod_stats['rtf']
    if min_prod_rtf <= prod_rtf <= max_prod_rtf:
        pass
    else:
        invalid_prod = True
        message.append('Prod RTF %f is out of bounds [%f, %f]' % (prod_rtf, min_prod_rtf, max_prod_rtf))
    test_rtf = test_stats['rtf']
    if min_test_rtf <= test_rtf <= max_test_rtf:
        pass
    else:
        invalid_test = True
        ship_it = False
        message.append('Test RTF %f is out of bounds [%f, %f]' % (test_rtf, min_test_rtf, max_test_rtf))
    message.append('RTF: %f (%f in prod)' % (test_rtf, prod_rtf))

    prod_wer = prod_stats['yandex']
    if min_prod_wer <= prod_wer <= max_prod_wer:
        pass
    else:
        invalid_prod = True
        message.append('Prod WER %f is out of bounds [%f, %f]' % (prod_wer, min_prod_wer, max_prod_wer))
    test_wer = test_stats['yandex']
    if min_test_wer <= test_wer <= max_test_wer:
        pass
    else:
        invalid_test = True
        message.append('Test WER %f is out of bounds [%f, %f]' % (test_wer, min_test_wer, max_test_wer))
    message.append('WER: %f%% (%f%% in prod)' % (test_wer, prod_wer))

    test_uer = test_stats['utterances_with_errors'] * 100.0 / test_stats['utterances']
    prod_uer = prod_stats['utterances_with_errors'] * 100.0 / prod_stats['utterances']
    message.append('UER: %f%% (%f%% in prod)' % (
        test_uer,
        prod_uer
    ))

    pvalue = wilcoxon(prod_error_utterances, test_error_utterances, "zsplit").pvalue
    message.append('pvalue: %f' % pvalue)
    if pvalue >= min_pvalue:
        message.append('There is no significant difference in UER on this test set')
    elif test_uer < prod_uer:
        message.append('Test is better then prod according to UER')
    else:
        invalid_test = True
        message.append('Prod is significantly better according to UER!')
    if invalid_prod:
        message.append('Something wrong with production in this comparison! Please check.\n' +
                       'If everything is actually ok, change operation params.')
    if invalid_test:
        message.append('Something wrong with testing in this comparision! \n' +
                       'Either model is worse than prod or measurement incorrect')
    if (not invalid_test) and (not invalid_prod):
        message.append('New model can go to production')

    result = {
        'text': '\n'.join(message),
        'prod_stats': prod_stats,
        'test_stats': test_stats,
        'to_prod': (not invalid_test) and (not invalid_prod),
        'bad_testing': invalid_test,
        'bad_production': invalid_prod,
        'pvalue': pvalue
    }
    return result


if __name__ == '__main__':
    call_as_operation(
        main,
        input_spec={
            'prod_stats': {
                'link_name': 'prod_stats',
                'required': True, 'parser': 'json'
            },
            'test_stats': {
                'link_name': 'test_stats',
                'required': True, 'parser': 'json'
            },
            'test_error_utterances': {
                'link_name': 'test_error_utterances',
                'required': True, 'parser': 'json'
            },
            'prod_error_utterances': {
                'link_name': 'prod_error_utterances',
                'required': True, 'parser': 'json'
            }
        }
    )

