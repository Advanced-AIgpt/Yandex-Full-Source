import operations.irover_aggregation.utils.metrics as metrics
from utils.nirvana.op_caller import call_as_operation
import sys


def main(data):
    if sys.version_info >= (3, ):
        data = [(x["ref"], x["hyp"]) for x in data]
    else:
        data = [(x["ref"].encode("utf8"), x["hyp"].encode("utf8") if x["hyp"] is not None else x["hyp"]) for x in data]
    recall, wer = metrics.evaluate_metrics_from_texts(data)
    result = {
        'wer': wer,
        'recall': recall,
        'records': len(data),
    }
    return result


if __name__ == '__main__':
    call_as_operation(main, input_spec={'data': {'required': True, 'parser': 'json'}})
