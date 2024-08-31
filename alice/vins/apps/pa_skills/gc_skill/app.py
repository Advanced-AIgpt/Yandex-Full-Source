# coding: utf-8
from __future__ import unicode_literals

from vins_sdk.app import VinsApp, callback_method
from personal_assistant.general_conversation import GeneralConversation
from personal_assistant.app import GeneralConversationSourceMeta
from vins_core.dm.response import ActionButton, ClientActionDirective
from vins_core.utils.config import get_setting
from vins_core.utils.data import load_data_from_file
import numpy


class ExternalSkillApp(VinsApp):
    def __init__(self, seed=None, *args, **kwargs):
        super(ExternalSkillApp, self).__init__(*args, **kwargs)
        self._gc = GeneralConversation()
        topics = load_data_from_file('gc_skill/config/topics.yaml')
        self._topics = [topic["topic"] for topic in topics]
        self._topic_proba = numpy.array([topic["popularity"] for topic in topics], dtype=float)
        self._topic_proba /= numpy.sum(self._topic_proba)
        self._topic_suggest_proba = float(get_setting('GC_TOPIC_SUGGEST_PROBA', 1))
        self._rng = numpy.random.RandomState(seed)

    def _generate_topic(self):
        return self._rng.choice(self._topics, p=self._topic_proba)

    @callback_method
    def general_conversation(self, req_info, session, response, sample, form, **kwargs):
        if req_info.reset_session:
            long_welcome = not session.get('long_welcome_set')
            session.set('long_welcome_set', True)

            suggest_topic = req_info.experiments['gc_skill_suggest_topic'] is not None
            if suggest_topic:
                suggest_topic = self._rng.rand() < self._topic_suggest_proba
            phrase = self.render_phrase(
                phrase_id='conversation_start',
                form=form,
                req_info=req_info,
                context={'long_welcome': long_welcome,
                         'suggest_topic': self._generate_topic() if suggest_topic else None},
                session=session,
            )
        else:
            resp, suggests = self._gc.get_response_with_suggests(req_info, session, sample)

            gc_phrase, source = None, None
            if resp is not None:
                gc_phrase = resp.text
                source = resp.source

            phrase = self.render_phrase(
                phrase_id='gc_response', form=form, context={'gc': gc_phrase}, req_info=req_info, session=session
            )
            if source is not None:
                response.add_meta(GeneralConversationSourceMeta(source=source))

            for suggest in suggests:
                caption = self.render_phrase(
                    phrase_id='render_suggest',
                    form=form,
                    context={'suggest': suggest},
                    req_info=req_info,
                    session=session,
                ).text

                response.suggests.append(
                    ActionButton(
                        title=caption,
                        directives=[
                            ClientActionDirective(
                                name='type',
                                sub_name='general_conversation_type',
                                payload={'text': caption}
                            )
                        ]
                    )
                )

        response.say(phrase.voice, phrase.text)

    @callback_method
    def handcrafted(self, response, form, req_info, session, **kwargs):
        if req_info.reset_session:
            phrase = self.render_phrase(phrase_id='conversation_start', form=form, req_info=req_info, session=session)
        else:
            phrase = self.render_phrase(phrase_id='render_result', form=form, req_info=req_info, session=session)

        response.say(phrase.voice, phrase.text)
