import json
import library.python.resource
from alice.analytics.wer.metrics_counter import calculate_metrics

CR = json.loads(library.python.resource.find('/cr.json'))
SENSE = set(library.python.resource.find('/sense.txt').decode().split("\n"))
STOP = set(library.python.resource.find('/stop.txt').decode().split("\n"))


def test_metrics():
    ref = 'мама мыла раму'
    hyp = ref
    metrics = calculate_metrics(ref, hyp, stop_words=STOP,
                                sense_words=SENSE, cluster_references=CR,
                                WER_star_v00_weights=None, WER_star_v11_weights=None)
    assert metrics['WER*v00'] == 0
    assert metrics['WER*v11'] == 0

    ref = 'сколько осталось времени'
    hyp = 'сколько времени осталось'
    metrics = calculate_metrics(ref, hyp, stop_words=STOP,
                                sense_words=SENSE, cluster_references=CR,
                                WER_star_v00_weights=None, WER_star_v11_weights=None)
    assert metrics['WER*v00'] == 0.016666666666666635
    assert metrics['WER*v11'] == 0.6666666666666666

    ref = 'алиса найди в сети том и джерри'
    hyp = 'алиса найди сети том и джерри'
    metrics = calculate_metrics(ref, hyp, stop_words=STOP,
                                sense_words=SENSE, cluster_references=CR,
                                WER_star_v00_weights=None, WER_star_v11_weights=None)
    assert metrics['WER*v00'] == -0.05285714285714285
    assert metrics['WER*v11'] == 0
