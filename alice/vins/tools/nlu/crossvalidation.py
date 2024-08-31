# coding: utf-8
from __future__ import unicode_literals

import argparse
import json
import sys
import time
import datetime
import numpy as np
from operator import itemgetter

from vins_core.app_utils import load_app
from vins_core.dm.session import Session
from vins_core.dm.intent import Intent

from sklearn.metrics.classification import classification_report, precision_recall_fscore_support

APPS = [
    'personal_assistant',
    'navi_app',
]


def _split_sample_features(sample_features, rng, train_ratio=0.8):
    train_features, validation_features = {}, {}
    for intent_name in sorted(sample_features):
        intent_features = sample_features[intent_name]
        rng.shuffle(intent_features)
        train_features[intent_name] = intent_features[:int(train_ratio * len(intent_features))]
        validation_features[intent_name] = intent_features[int(train_ratio * len(intent_features)):]
    return train_features, validation_features


def _predict_intents(feature, session, nlu):
    for level, classifiers in enumerate(nlu._intent_classifiers):
        likelihoods = nlu.compute_likelihoods(classifiers, nlu._classifiers_pool, feature)
        posteriors = nlu.compute_posteriors(
            likelihoods, session, None,
            nlu._intent_infos, nlu._transition_model
        )
        intent_candidates = sorted(posteriors.iteritems(), key=itemgetter(1), reverse=True)

        intent_winner, winner_score = None, 0
        for intent_candidate, score in intent_candidates:
            if score > nlu._intent_infos[intent_candidate].fallback_threshold and score > winner_score:
                winner_score = score
                intent_winner = intent_candidate
        if intent_winner:
            return [intent_winner], [winner_score]

    return [nlu._fallback_intent], [1.0]


def _crossvalidation(nlu, exclude_names, **kwargs):
    sample_features = nlu._extract_features_on_train(nlu._input_data)
    print "Features extracted from %d intents" % len(sample_features)

    print "Start training..."
    rng = np.random.RandomState(42)
    train_features, validation_features = _split_sample_features(sample_features, rng)

    nlu._train_intent_classifiers(train_features)
    pred_intents, true_intents = [], []
    print 'Start validation...'
    for intent_name, features in validation_features.iteritems():
        if any(name in intent_name for name in exclude_names):
            continue
        session = Session('123', '123')

        if '__' in intent_name:
            session.change_intent(Intent(intent_name.split('__')[0]))

        for feature in features:
            pred_intent, pred_confidence = _predict_intents(feature, session, nlu)
            print '%s --> %s' % (intent_name, pred_intent[0])
            pred_intents.append(pred_intent[0])
            true_intents.append(intent_name)

    p, r, f, s = precision_recall_fscore_support(true_intents, pred_intents, average=None)
    classifier_cvinfo = {'mean': np.mean(f[np.where(s > 0)]), 'std': np.std(f[np.where(s > 0)])}

    print "Start utterance tagger crossvalidation"
    tagger_cvinfo = nlu._create_utterance_tagger().crossvalidation(
        nlu._create_train_data_for_utterance_tagger(sample_features),
        **kwargs
    )
    print classification_report(true_intents, pred_intents)
    return {
        'classifier': classifier_cvinfo,
        'tagger': tagger_cvinfo
    }


def crossvalidation(app_name, **kwargs):
    app = load_app(app_name, force_rebuild=False, load_data=True, load_model=False)
    return _crossvalidation(app.nlu, **kwargs)


def save(result, app_name):
    time_now = datetime.datetime.now()
    out = {
        'app': app_name,
        'datetime': time_now.strftime('%c'),
        'timestamp': int(time.mktime(time_now.timetuple()))
    }
    out.update(result)
    sys.stdout.write(json.dumps(out, sort_keys=True))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--app', dest='app', required=True, choices=APPS,
                        help='The name of the app to test')
    parser.add_argument('--average', dest='average', required=False, choices=['micro', 'macro', 'weighted'],
                        default='micro', help='F-score averaging method')
    parser.add_argument('--njobs', dest='njobs', required=False, default=1, type=int,
                        help='Parallel jobs count')
    parser.add_argument('--cv', dest='cv', required=False, default=3, type=int,
                        help='Number of cross-validation splits')
    parser.add_argument('--output-file', dest='output_file', required=False,
                        help='Output file to dump error statistics')
    parser.add_argument('--exclude', nargs='+', help='intent name keywords to exclude', default=())
    args = parser.parse_args()

    result = crossvalidation(
        args.app,
        average=args.average,
        n_jobs=args.njobs,
        cv=args.cv,
        output_file=args.output_file,
        exclude_names=args.exclude
    )
    save(result, args.app)


if __name__ == "__main__":
    main()
