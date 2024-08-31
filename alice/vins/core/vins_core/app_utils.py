# coding: utf-8

from vins_core.utils.data import find_vinsfile, logger
from vins_core.dm.intent import Intent
from vins_core.utils.datetime import utcnow
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.dm.request import create_request, AppInfo


def load_app(
    app_name, load_data=True, load_model=True, **kwargs
):
    from vins_core.config.app_config import AppConfig
    from vins_core.dm.form_filler.dialog_manager import DialogManager

    vinsfile = find_vinsfile(app_name)
    app_conf = AppConfig()
    app_conf.parse_vinsfile(vinsfile)

    logger.info('App loading')
    return DialogManager.from_config(
        app_conf,
        load_data=load_data,
        load_model=load_model,
        **kwargs
    )


def compile_nlu(nlu, archive, classifiers, taggers, feature_cache, skip_train, **kwargs):
    if not skip_train:
        logger.info('Start NLU training...')
        nlu.train(classifiers, taggers, feature_cache=feature_cache, **kwargs)
        logger.info('NLU training have finished')
    logger.info('NLU saving')
    nlu.validate()
    nlu.save(archive)


def compile_app(app_name, archive, classifiers=None, taggers=True, feature_cache=None, skip_train=False, **kwargs):
    """
    :param app_name: name of the app to compile
    :param archive: Archive object to save compiled models
    :param kwargs:
    :param classifiers: Specify the list of classifiers to update
    :param taggers: Whether to retrain taggers
    :param skip_train: If set, skip training classifiers & taggers
    :param feature_cache: File path to store / retrieve precomputed train features
    :return:
    """
    dm = load_app(app_name, **kwargs)
    compile_nlu(dm.nlu, archive, classifiers, taggers, feature_cache, skip_train)


def get_nlu_result(app, utterance, req_info=None, prev_intent=None, app_id=None, client_time=None, sample=None,
                   skip_intents=None, **kwargs):
    client_time = client_time or utcnow()
    if not req_info:
        req_info = create_request(
            uuid=gen_uuid_for_tests(),
            utterance=utterance,
            client_time=client_time,
            app_info=AppInfo(app_id=app_id)
        )
    session = app.load_or_create_session(req_info).clear()
    if skip_intents is not None:
        session.set('skip_intents', skip_intents, transient=True)

    if prev_intent:
        session.change_intent(Intent(prev_intent))
    sample = app.samples_extractor([sample or utterance], session, filter_errors=True)[0]
    return sample, app.dm.get_semantic_frames(
        sample, session, req_info=req_info, **kwargs
    )


def get_app_response(app, utterance, req_info=None, prev_intent=None, app_id=None, client_time=None, **kwargs):
    client_time = client_time or utcnow()
    if not req_info:
        req_info = create_request(
            uuid=gen_uuid_for_tests(),
            utterance=utterance,
            client_time=client_time,
            app_info=AppInfo(app_id=app_id)
        )
    session = app.load_or_create_session(req_info).clear()

    if prev_intent:
        session.change_intent(Intent(prev_intent))

    response = app.handle_request(req_info, session=session)
    return response, session.form
