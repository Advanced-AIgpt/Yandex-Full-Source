import importlib
import json
import logging
import time

from vins_core.config.app_config import AppConfig, Project

from vins_core.dm.request import create_request, AppInfo, get_experiments, configure_experiment_flags
from vins_core.dm.response import VinsResponse
from vins_core.dm.form_filler.events import RestorePrevFormEventHandler
from vins_core.app_utils import get_nlu_result, get_app_response
from vins_core.utils.datetime import utcnow
from vins_core.utils.misc import parallel, gen_uuid_for_tests
from vins_core.common.slots_map_utils import tags_to_slots


from vins_tools.nlu.inspection.dataset_iteration import iterate_file
from vins_tools.nlu.inspection.nlu_result_info import (
    NluResultInfo, UtteranceInfo, NluResult, ProcessedItem, save_items, AppResult
)


logger = logging.getLogger(__name__)

APP_ALIASES = {
    'personal_assistant': 'personal_assistant.app::PersonalAssistantApp',
    'navi_app': 'navi_app.app::NaviApp',
    'gc_skill': 'gc_skill.app::ExternalSkillApp',
    'crm_bot': 'crm_bot.app::CrmBotApp',
}


def get_vinsfile_for_app(app_name):
    VINSFILES = {
        'personal_assistant': (
            'personal_assistant/config/Vinsfile.json',
            'personal_assistant/config/Vinsfile.json'
        ),
        'gc_skill': (
            'gc_skill/config/Vinsfile.json',
            'gc_skill/config/Vinsfile.json'
        ),
        'navi': (
            'navi_app/config/Vinsfile.json',
            'navi_app/config/Vinsfile.json'
        ),
        'crm_bot': (
            'crm_bot/config/Vinsfile.json',
            'crm_bot/config/Vinsfile.json'
        )
    }
    return VINSFILES[app_name]


def _create_app_config_with_custom_intents(vins_file, custom_intents_list, keep_vinsfile_projects):
    app_conf = AppConfig()
    app_conf.parse_vinsfile(vins_file)
    head_project_name = app_conf.projects[0].name
    logger.info('The default vinsfile %s has been parsed', vins_file)

    if not keep_vinsfile_projects:
        for project in app_conf.projects:
            # intents is a write-only property -> remove all elements instead of replacing it
            project.intents[:] = []
    for custom_intents in custom_intents_list:
        logger.info(
            'Creating custom app config by using custom intents=%s',
            custom_intents
        )
        with open(custom_intents, 'r') as f:
            dummy_project_dict = json.load(f)
        for intent in dummy_project_dict.get('intents', []):
            if 'dm' not in intent:
                intent['dm'] = {'data': {'name': 'dummy_dm'}}
        new_project = Project.from_dict(
            dummy_project_dict,
            base_name=head_project_name,
            nlu_templates=app_conf.nlu_templates
        )
        # this patch includes both normal intents and microintents (the latter are not covered in dummy_project_dict)
        for intent in new_project.intents:
            intent.nlg_filename = None
            if intent.nlg:
                intent.nlg_sources = None
        app_conf.projects.append(new_project)
    return app_conf


def create_app(
    app_name_or_path, vins_file=None, load_data=True,
    custom_intents=None, keep_vinsfile_projects=False, **kwargs
):
    # todo(ddale): cleanup all the mess with app path, module path and vins_file
    app_path = APP_ALIASES.get(app_name_or_path, app_name_or_path)
    module_name, app_class_name = app_path.split('::')
    if vins_file is None:
        vins_file_original_path, vins_file = get_vinsfile_for_app(app_name_or_path)
    app_class = getattr(importlib.import_module(module_name), app_class_name)
    logger.info('%s imported from %s', app_class_name, module_name)
    if custom_intents:
        app_conf = _create_app_config_with_custom_intents(vins_file, custom_intents, keep_vinsfile_projects)
    else:
        logger.info('Creating app config from native project intents')
        app_conf = None

    app = app_class(
        vins_file=vins_file,
        app_conf=app_conf,
        load_data=load_data,
        **kwargs
    )
    return app


def load_app_by_name(app_name, vins_file=None):
    app = create_app(app_name, vins_file=vins_file, load_data=False, allow_wizard_request=True)
    if not app.nlu.trained:
        logger.info('Trying to create app %s but models are not trained. Rebuilding model...')
        app = create_app(app_name, load_data=True, allow_wizard_request=True)
        app.nlu.train()
    return app, app_name


def get_processed_item(
    item, app, nbest, force_intent, app_info_kwargs=None, device_state=None, experiments=None, use_bass=False,
    apply_item_selection=False,
    **kwargs
):
    utterance, info, sample = item
    if not device_state:
        device_state = info.device_state or {}
    app_info_kwargs = app_info_kwargs or {}
    request_experiments = get_experiments()
    if experiments:
        request_experiments = configure_experiment_flags(request_experiments, experiments)
    req_info = create_request(
        uuid=gen_uuid_for_tests(),
        utterance=utterance,
        client_time=utcnow(),
        app_info=AppInfo(**app_info_kwargs),
        device_state=device_state,
        experiments=request_experiments
    )
    app_result = None
    if use_bass:
        try:
            _, form = get_app_response(
                app, utterance, req_info=req_info, prev_intent=info.prev_intent,
                max_intents=nbest, force_intent=force_intent,
            )
            app_result = AppResult(
                intent_name=form.name,
            )
        except Exception:
            pass

    sample, nlu_result = get_nlu_result(
        app, utterance, req_info=req_info, prev_intent=info.prev_intent,
        max_intents=nbest, force_intent=force_intent,
        sample=sample
    )
    selected_item = None
    if apply_item_selection:
        intent_name = nlu_result.semantic_frames[0]['intent_name']
        f = app.dm.new_form(intent_name)
        if 'on_item_selection' in {h.to_dict().get('name') for e in f.events for h in e.handlers}:
            session = app.load_or_create_session(req_info).clear()
            form = app.dm._apply_frame_to_form(
                app, response=None, sample=sample, session=session, req_info=req_info,
                frame=nlu_result.semantic_frames[0]
            )
            session.change_form(form)

            update = {}

            def mocked_change_form(session, new_form, req_info, sample, response):
                if new_form.name != intent_name:
                    logger.debug('Form has been changed to {}'.format(new_form.name))
                    update['form_name'] = new_form.name
                if 'video_index' in new_form:
                    update['selected_item'] = new_form.video_index.value

            def mocked_abort_item_selection(sample, req_info, session, form, response):
                RestorePrevFormEventHandler().handle(session, app, form, req_info, response, add_overriden_meta=False)
                logger.debug('Item selection has been aborted')
                sample, nlu_result = get_nlu_result(
                    app, utterance, req_info=req_info, prev_intent=info.prev_intent,
                    max_intents=nbest, force_intent=force_intent,
                    sample=sample, skip_intents=[intent_name]
                )
                update['nlu_result'] = nlu_result

            original_methods = app.change_form, app._abort_item_selection
            app.change_form, app._abort_item_selection = mocked_change_form, mocked_abort_item_selection
            app.on_item_selection(req_info=req_info, session=session, form=form, response=VinsResponse(), sample=sample)
            app.change_form, app._abort_item_selection = original_methods

            if 'form_name' in update:
                nlu_result.semantic_frames[0]['intent_name'] = update['form_name']
            if 'nlu_result' in update:
                nlu_result = update['nlu_result']
            if 'selected_item' in update:
                selected_item = update['selected_item']

    if sample.has_tags:
        info = UtteranceInfo(
            slots=tags_to_slots(sample.tokens, sample.tags)[0],
            true_intent=info.true_intent,
            prev_intent=info.prev_intent,
            additional_data=info.additional_data
        )
    return ProcessedItem(
        utterance=utterance,
        utterance_info=info,
        nlu_result=NluResult(
            semantic_frames=nlu_result.semantic_frames,
            input_sample=sample,
            selected_item=selected_item
        ),
        app_result=app_result,
    )


def do_classify(
    app_name, input_file, format, output_file, nbest, force_intent, text_col, intent_col,
    prev_intent_col=None, weight_col=None, device_state_col=None, vinsfile=None, app_info_kwargs=None,
    device_state=None, ignore_errors=False, experiments=(), use_bass=False, preloaded_app=None, num_procs=None,
    additional_columns=None, apply_item_selection=False
):
    """
    :param input_file: input file (.txt, .xls)
    :param output_file: optional output file to dump results (otherwise prints to stdout)
    """
    device_state = device_state or {}
    logger.info('Running intent classification for %s on %s', app_name, input_file)
    total_items = list(
        iterate_file(input_file, format, text_col=text_col, intent_col=intent_col, prev_intent_col=prev_intent_col,
                     weight_col=weight_col, device_state_col=device_state_col, additional_columns=additional_columns)
    )
    logger.info('%d items have been loaded', len(total_items))
    if preloaded_app:
        app = preloaded_app
        logger.info('App %s has been pre-loaded.', app_name)
    else:
        app, app_name = load_app_by_name(app_name, vinsfile)
        logger.info('App %s loaded.', app_name)
    time_st = time.time()
    processed_items = parallel(
        function=get_processed_item,
        items=total_items,
        function_kwargs={
            'nbest': nbest, 'force_intent': force_intent, 'app_info_kwargs': app_info_kwargs,
            'experiments': experiments, 'app': app, 'app_name': app_name, 'device_state': device_state,
            'use_bass': use_bass, 'apply_item_selection': apply_item_selection
        },
        filter_errors=ignore_errors,
        raise_on_error=not ignore_errors,
        num_procs=num_procs
    )
    save_items(processed_items, output_file)
    time_el = time.time() - time_st
    logger.info('Done. Time elapsed: %.2f sec. (%.4f sec average item processing)' % (
        time_el, float(time_el) / len(total_items)))


def do_report(**kwargs):
    NluResultInfo(**kwargs).make_reports()


def do_crossval(app_name, output_file, num_runs, classifiers, taggers, feature_cache, validation, validate_intent):
    app = create_app(app_name)
    classifiers_processed_items = []
    taggers_processed_items = []
    for run_index in xrange(num_runs):
        logger.info('Start training / validation run #%d' % run_index)
        train_results = app.nlu.train(
            classifiers=classifiers, taggers=taggers, feature_cache=feature_cache,
            validation=1 - validation, validate_intent=validate_intent
        )
        if train_results['classifiers_validation_result']:
            for sample_features, true_intent, semantic_frames in train_results['classifiers_validation_result']:
                classifiers_processed_items.append(ProcessedItem(
                    utterance=sample_features.sample.utterance,
                    utterance_info=UtteranceInfo(true_intent=true_intent),
                    nlu_result=NluResult(
                        semantic_frames=semantic_frames,
                        input_sample=sample_features.sample
                    )
                ))
        if train_results['taggers_validation_result']:
            for sample_features, semantic_frames in train_results['taggers_validation_result']:
                taggers_processed_items.append(ProcessedItem(
                    utterance=sample_features.sample.utterance,
                    utterance_info=UtteranceInfo(
                        slots=tags_to_slots(sample_features.sample.tokens, sample_features.sample.tags)[0]
                    ),
                    nlu_result=NluResult(
                        semantic_frames=semantic_frames,
                        input_sample=sample_features.sample
                    )
                ))
    if classifiers_processed_items:
        save_items(classifiers_processed_items, output_file + '.classifiers.pkl')
    if taggers_processed_items:
        save_items(taggers_processed_items, output_file + '.taggers.pkl')
