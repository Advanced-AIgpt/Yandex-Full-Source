# -*- coding: utf-8 -*-

""" Hopefully temporal solution for the reranker dataset collection
"""

import click
import json
import os
import time
import numpy as np

from vins_core.nlu.features.extractor.base import DenseFeatures
from vins_core.nlu.base_nlu import FeatureExtractorResult
from vins_core.dm.request import create_request, AppInfo
from vins_core.dm.session import Session
from vins_core.dm.intent import Intent
from vins_core.dm.formats import NluSourceItem
from vins_core.dm.form_filler.form_candidate import FormCandidate
from vins_core.dm.response import VinsResponse
from vins_core.utils.datetime import utcnow
from vins_core.utils.misc import gen_uuid_for_tests, parallel

from personal_assistant.setup_feature_updater import BassSetupFeatureUpdater

from dataset import VinsDatasetBuilder, AdditionalInfo
from train_word_nn import load_intent_normalizations


def _process_item(item, app, intent_to_index, app_info_kwargs=None, experiments=None, **kwargs):
    utterance, info = item[:2]
    device_state = info.device_state or {}

    app_info_kwargs = app_info_kwargs or {}
    req_info = create_request(
        uuid=gen_uuid_for_tests(),
        utterance=utterance,
        client_time=utcnow(),
        app_info=AppInfo(**app_info_kwargs),
        device_state=device_state,
        experiments=experiments or ()
    )

    session = Session(req_info.app_info.app_id, req_info.uuid)
    session.change_intent(Intent(info.prev_intent))

    sample = app.samples_extractor([utterance], session, filter_errors=True)[0]
    nlu_result = app.dm.get_semantic_frames(
        sample, session, req_info=req_info, max_intents=100
    )

    form_candidates = []
    for index, frame in enumerate(nlu_result.semantic_frames):
        form = app.dm._apply_frame_to_form(app, VinsResponse(), sample, session, req_info, frame, **kwargs)
        form_candidates.append(
            FormCandidate(form=form, intent=frame.get('intent_candidate'), frame=frame, index=index)
        )

    feature_updater = BassSetupFeatureUpdater()
    sample_feature_updater_result = feature_updater.update_features(
        app, nlu_result.sample_features, session, req_info, form_candidates
    )

    sample_features = sample_feature_updater_result.sample_features

    scenarios_scores_feature = np.zeros(len(intent_to_index))
    tagger_scores_feature = np.zeros(len(intent_to_index))
    for frame in nlu_result.semantic_frames:
        candidate = frame.get('intent_candidate')
        candidate_index = intent_to_index.get(candidate.name_for_reranker, None)
        if candidate_index is None:
            continue

        scenarios_scores_feature[candidate_index] = max(scenarios_scores_feature[candidate_index], candidate.score)

        tagger_score = frame.get('tagger_score', 1.)
        tagger_score = tagger_score if tagger_score is not None else 1.
        tagger_scores_feature[candidate_index] = max(tagger_scores_feature[candidate_index], tagger_score)

    scenarios_scores_feature = DenseFeatures(data=scenarios_scores_feature)
    sample_features.add(scenarios_scores_feature, 'scenarios')

    tagger_scores_feature = DenseFeatures(data=tagger_scores_feature)
    sample_features.add(tagger_scores_feature, 'tagger_scores')

    return utterance, info, sample_features, {}, {}


def _collect_dataset(
    input_file, output_file, target_classifier='scenarios', text_col='text', intent_col='intent',
    prev_intent_col='prev_intent', device_state_col='device_state', app_info=None,
    experiments=(), rename=True, intent_to_index_path=None
):
    from compile_app_model import create_app
    from process_nlu_on_dataset import iterate_file

    total_items = list(
        iterate_file(input_file, format='tsv', text_col=text_col, intent_col=intent_col,
                     prev_intent_col=prev_intent_col, device_state_col=device_state_col)
    )
    click.echo('{} items have been loaded'.format(len(total_items)))

    app = create_app('personal_assistant', load_data=False)

    with open(intent_to_index_path) as f:
        intent_to_index = json.load(f)

    click.echo('App has been loaded')

    app_info = json.loads(app_info)

    start_time = time.time()
    sample_features_list = parallel(
        function=_process_item,
        items=total_items,
        function_kwargs={
            'app': app, 'intent_to_index': intent_to_index, 'app_info_kwargs': app_info, 'experiments': experiments
        },
        filter_errors=True,
        raise_on_error=False
    )

    click.echo('Items were processed in {:.2f} sec'.format(time.time() - start_time))

    renamer = load_intent_normalizations() if rename else lambda intent: intent

    sample_infos = []
    for text, info, sample_features, slots, raw_factors_data in sample_features_list:
        item = NluSourceItem(
            text=text, original_text=text,
            trainable_classifiers=[target_classifier],
            can_use_to_train_tagger=False
        )
        feature_extraction_result = FeatureExtractorResult(item=item, sample_features=sample_features)
        sample_infos.append((
            renamer(info.true_intent),
            feature_extraction_result,
            AdditionalInfo(device_state=info.device_state, prev_intent=info.prev_intent,
                           slots=slots, raw_factors_data=raw_factors_data)
        ))

    dataset = VinsDatasetBuilder(sample_infos=sample_infos).build()
    dataset.add_classifier_feature_mapping('scenarios', intent_to_index)
    dataset.save(output_file)

    click.echo('Saved the dataset to {}'.format(output_file))


@click.command()
@click.option('-i', '--input-file', type=click.Path(exists=True))
@click.option('-o', '--output-file', help='output file to dump the results', default=None)
@click.option('--target-classifier', help='name of classifier to be used as a key in VinsDataset', default='scenarios')
@click.option('--text-col', help='text column name', default='text')
@click.option('--intent-col', help='intent column name', default='intent')
@click.option('--prev-intent-col', help='previous intent column name', default='prev_intent')
@click.option('--device-state-col', help='json encoded device state column name', default='device_state')
@click.option('--app-info', help='JSON with AppInfo kwargs', default='{"app_id": "ru.yandex.quasar.services"}')
@click.option('--experiments', help='Experiments flags separated by commas',
              default='video_play,how_much,avia,ambient_sound,music_video_setup,'
                      'music_use_websearch,force_intents,bass_setup_features')
@click.option('--rename', is_flag=True, default=True)
@click.option('--intent-to-index', type=click.Path(exists=True), default='tools/train/configs/intent_to_index.json')
def main(
    input_file, output_file, target_classifier='scenarios', text_col='text', intent_col='intent',
    prev_intent_col='prev_intent', device_state_col='device_state', app_info=None,
    experiments=(), rename=True, intent_to_index=None
):
    import sys
    sys.path.append(os.path.join(os.path.dirname(__file__), os.pardir, 'nlu'))
    from process_nlu_on_dataset import set_logging

    os.environ['VINS_LOAD_TF_ON_CALL'] = '1'
    set_logging('WARNING')

    _collect_dataset(
        input_file, output_file, target_classifier, text_col, intent_col, prev_intent_col,
        device_state_col, app_info, experiments.split(','), rename, intent_to_index
    )


if __name__ == '__main__':
    main()
