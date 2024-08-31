# coding: utf-8
from __future__ import unicode_literals


from crm_bot import intents
from crm_bot.api.crm_bot import CrmBotAPI
from personal_assistant import bass_session_state
from personal_assistant.app import (
    PersonalAssistantApp, PersonalAssistantDialogManager, PersonalAssistantErrorBlockError
)
from personal_assistant.blocks import (
    ErrorBlock, SuggestBlock, AnalyticsInfoBlock, StopListeningBlock,
    AutoactionDelayMsBlock, SilentResponseBlock, TextCardBlock
)
from personal_assistant.bass_result import BassReportResult
from personal_assistant.callback import bass_result_key, callback_method
from vins_core.dm.form_filler.models import Form
from vins_core.dm.response import ActionButton, ServerActionDirective
from vins_core.nlu.features.base import IntentScore
from vins_core.utils.metrics import sensors

from logging import getLogger

logger = getLogger(__name__)


class CrmBotDialogManager(PersonalAssistantDialogManager):
    def get_suggests(self, intent_name, req_info):
        assert self.has_intent(intent_name), 'Intent name {} not found in the dialog manager'.format(intent_name)
        return self.get_intent(intent_name).suggests or []


class CrmBotApp(PersonalAssistantApp):
    dm_class = CrmBotDialogManager

    def __init__(self, **kwargs):
        super(CrmBotApp, self).__init__(**kwargs)
        self._pa_api = CrmBotAPI()

    def handle_request(self, req_info, **kwargs):
        with sensors.timer('app_load_session_time'):
            session = self._load_session(req_info)

        # doing it here and not in connector so we don't load session from mongo twice
        personal_data = session.get('visitor_fields', {})
        sec_data = session.get('sec_data', None)
        if sec_data is not None:
            personal_data['secure'] = sec_data
        if personal_data != {}:
            object.__setattr__(req_info, 'personal_data', personal_data)  # Note: req_info is frozen

        return super(CrmBotApp, self).handle_request(req_info, session=session, **kwargs)

    def _make_suggests_from_config(self, req_info, session):
        form = session.form
        intent_name = form and form.name

        regular_intent_suggests = []

        suggests = self.dm.get_suggests(intent_name, req_info)
        for suggest in suggests:
            if isinstance(suggest, basestring):
                suggest_type = suggest
                suggest_data = {}
            elif isinstance(suggest, dict):
                suggest_type = suggest['type']
                suggest_data = suggest.get('data', {})
            else:
                continue
            if suggest_type in ("redirect_me_button", "help_bot_button") and session.get('first_message', False):
                continue  # skip adding these verbose_redirect suggests if this is user's first message
            regular_intent_suggests.append(
                SuggestBlock(type='suggest', suggest_type=suggest_type, data=suggest_data)
            )

        return regular_intent_suggests

    def _generate_text_cards(self, blocks, form, context, response, req_info, session):
        text_cards = []
        for block in filter(lambda b: isinstance(b, TextCardBlock), blocks):
            logger.debug('Render text card block %s', block)

            if not self.has_phrase(phrase_id=block.phrase_id, intent_name=form.name):
                logger.warning(
                    'Don\'t know how to render text card block %s for intent %s',
                    block, form.name,
                )
                continue

            text_card_context = context.copy()
            text_card_context['data'] = block.data
            render_result = self.render_phrase(block.phrase_id, form=form, req_info=req_info,
                                               context=text_card_context, session=session)
            text_cards.append({
                'voice_text': render_result.voice or None,
                'card_text': render_result.text,
                'card_tag': (block.data or {}).get('card_tag'),
                'append': True,
            })
        return text_cards

    def _handle_response(self, bass_resp, response, session, req_info, sample, sample_features,
                         additional_nlg_context, analytics_info, raise_error=False):
        form, should_resubmit_form = self._update_form(session.form, req_info, bass_resp.form_info)

        if should_resubmit_form and sample_features is not None:
            intent_scores = [IntentScore(name=form.name)]
            sample_features.add_classification_scores('final_intent', intent_scores)

        self.dump_debug_info(sample, sample_features, req_info, response)

        # All NLG should be generated given of the updated form
        session.change_form(form)

        context = self._make_nlg_context(bass_resp.blocks, additional_nlg_context)
        context['user_info'] = req_info.personal_data
        client_features = self._parse_client_features(bass_resp.blocks)

        if should_resubmit_form:
            # If the form needs to be resubmitted, there should be no result rendering

            # Saving bass result in order to use it in following
            # _handle_response instead of geting real BASS response once again
            session.set(bass_result_key, bass_resp, transient=True)
            self.change_form(session=session, form=form, req_info=req_info, sample=sample, response=response)
            return

        # Add info about error and attention blocks to response
        self._add_block_info_to_response_meta(bass_resp.blocks, response, form.name if form else None)

        # Check if BASS altered the session state.
        if bass_resp.session_state is not None:
            bass_session_state.set_bass_session_state(session, bass_resp.session_state)

        # See if there are any error blocks
        error_blocks = [block for block in bass_resp.blocks if isinstance(block, ErrorBlock)]
        for error_block in error_blocks:
            self._render_error_block(session.form, error_block, context, req_info, response)
            sensors.inc_counter('pa_bass_error_block', labels={'error_type': error_block.error_type})

        if error_blocks and raise_error:
            raise PersonalAssistantErrorBlockError(error_blocks)

        # Build suggests from VinsProjectfile
        config_suggests_blocks = self._make_suggests_from_config(req_info, session)

        # Produce directives, buttons and div-cards
        suggests, card_buttons = self._render_buttons(req_info, response, session, config_suggests_blocks + bass_resp.blocks,
                                                      context, client_features)
        div_cards = self._make_cards(req_info, response, session.form, context, bass_resp.blocks)
        directives = self._make_directives(req_info, bass_resp.blocks)

        special_buttons = []

        # And add them to the response
        response.suggests += suggests
        response.directives += directives
        response.special_buttons += special_buttons

        if analytics_info is not None:
            for block in bass_resp.blocks:
                if isinstance(block, AnalyticsInfoBlock):
                    analytics_info.scenario_analytics_info_data = block.data

        if any(isinstance(block, StopListeningBlock) for block in bass_resp.blocks):
            response.should_listen = False

        for block in bass_resp.blocks:
            if isinstance(block, AutoactionDelayMsBlock):
                response.autoaction_delay_ms = block.delay_ms
                break

        # Do not render anything if there is SilentResponseBLock
        if any(isinstance(block, SilentResponseBlock) for block in bass_resp.blocks):
            response.should_listen = False
            return

        # Stop processing if there are any errors
        if error_blocks:
            return

        # Render result only if there are no errors

        text_and_voice_cards = self._generate_text_cards(bass_resp.blocks, form, context, response, req_info=req_info, session=session)
        card_with_buttons_id = 1

        if self.has_phrase(phrase_id='render_result', intent_name=form and form.name):
            postprocess_list = []

            render_result = self.render_phrase(
                'render_result', form=form, context=context, session=session,
                req_info=req_info, postprocess_list=postprocess_list
            )

            # render usual text message only if there is no div_cards
            if not div_cards:
                if render_result.text:
                    text_and_voice_cards.append({
                        "voice_text": render_result.voice or None,
                        "card_text": render_result.text,
                        "append": True,
                    })
                elif render_result.voice:
                    card_with_buttons_id += 1
                    text_and_voice_cards.append({
                        "voice_text": render_result.voice,
                        "card_text": False,
                        "append": True
                    })
            else:
                # otherwise render only voice
                if render_result.voice:
                    card_with_buttons_id += 1
                    text_and_voice_cards.append({
                        "voice_text": render_result.voice,
                        "card_text": False,
                        "append": True
                    })

        if len(text_and_voice_cards) >= card_with_buttons_id:
            text_and_voice_cards[-card_with_buttons_id]["buttons"] = card_buttons

        # Add greeting block if applicable
        intent_name = form and form.name
        if (
            intents.can_greet(intent_name) and
            session.get('first_request', False) and
            not session.get('doing_redirect', False)
        ):
            text_and_voice_cards = self._generate_text_cards(
                [TextCardBlock(type="text_card", phrase_id="greeting_addon", data={})],
                form, context, response, req_info=req_info, session=session
            ) + text_and_voice_cards

        # Remove verbose_redirect_suggests if redirecting silently
        if session.get('doing_redirect', False):
            response.suggests = [
                suggest for suggest in response.suggests
                if suggest.type not in ("redirect_me_button", "help_bot_button")
            ]
            new_suggests = []
            for suggest in response.suggests:
                skip = False
                if isinstance(suggest, ActionButton):
                    for directive in suggest.directives:
                        if isinstance(directive, ServerActionDirective):
                            try:
                                suggest_type = directive.payload['suggest_block']['suggest_type']
                            except (KeyError, TypeError):
                                suggest_type = None
                            if suggest_type in ("redirect_me_button", "help_bot_button"):
                                skip = True
                                break
                if not skip:
                    new_suggests.append(suggest)
            response.suggests = new_suggests

        # Add feedback block if applicable
        if intents.is_final_intent(intent_name) and not session.get('doing_redirect', False):
            text_and_voice_cards += self._generate_text_cards(
                [TextCardBlock(type="text_card", phrase_id="feedback_addon", data={})],
                form, context, response, req_info=req_info, session=session
            )
            feedback_suggests = [SuggestBlock(type='suggest', suggest_type='feedback_yes', data={}),
                                 SuggestBlock(type='suggest', suggest_type='feedback_no', data={"nobr": True})]
            suggests, card_buttons = self._render_buttons(req_info, response, session, feedback_suggests, context,
                                                          client_features)
            response.suggests += suggests

        # add text and voice cards, then div_cards
        self._say_cards(text_and_voice_cards, response)
        response.cards += div_cards

        # We should stop listening if the call results in a client action
        # (not always though)
        listening_is_possible = set()
        for directive in response.directives:
            if directive.payload and directive.payload.get('listening_is_possible'):
                listening_is_possible.add(directive.name)

        for directive in response.directives:
            if directive.name not in listening_is_possible:
                response.should_listen = False

        session.set('first_request', 0)  # pop first_request now in case it was not popped in nlg
        session.pop('doing_redirect')

    @callback_method
    def universal_callback_no_bass(self, req_info, session, response, sample,
                                   bass_result=None, analytics_info=None, **kwargs):
        bass_result = self._pa_api.fake_submit_form(bass_result or BassReportResult.empty(), req_info, session.form)

        self._handle_response(
            bass_result, response=response, session=session, req_info=req_info, sample=sample, sample_features=None,
            additional_nlg_context=None, analytics_info=analytics_info
        )

    def get_redirect_error_text(self, req_info=None):
        form = Form(name='crm_bot.scenarios.operator_redirect')
        render_result = self._dm.nlg.render_phrase('redirect_error', form=form, req_info=req_info)
        return render_result.text
