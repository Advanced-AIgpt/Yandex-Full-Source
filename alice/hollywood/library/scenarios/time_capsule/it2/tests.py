import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.scenario_responses import Accumulator


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['time_capsule']


@pytest.mark.scenario(name='TimeCapsule', handle='time_capsule')
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker, surface.searchapp])
@pytest.mark.experiments('hw_time_capsule_hardcode_session_id_exp')
class Tests:
    @pytest.mark.experiments('hw_time_capsule_enable_record_exp')
    @pytest.mark.oauth(auth.Yandex)
    def test_time_capsule_record(self, alice):
        test_responses = Accumulator()

        r = alice(voice('запиши капсулу времени'))
        test_responses.add(r)
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == \
            'С удовольствием. Спасибо, что доверяете мне самое важное. ' \
            'В этом году вы можете создать только одну капсулу времени, ' \
            'так что если хотите записать ее вместе с кем-то, позовите меня, когда соберетесь. ' \
            'Но не забудьте, что запись капсулы доступна до 17 января включительно. ' \
            'Готовы записать ее прямо сейчас?'
        r = alice(voice('да'))
        test_responses.add(r)

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == \
            'Отлично. Сейчас я задам вам несколько вопросов. ' \
            'Можете отвечать на все или говорить "пропусти вопрос". Поехали?'
        r = alice(voice('да'))
        test_responses.add(r)

        questions = [
            'Как вас зовут?',
            'Как настроение?',
            'Чем вы занимались сегодня?',
            'Вы влюблены в кого-нибудь сейчас?',
            'Расскажите о хорошем событии, которое недавно произошло.',
            'Кого вы хотите поблагодарить в уходящем году и за что?',
            'О чем вы мечтаете?',
            'Вы хорошо вели себя в этом году?',
            'Чем вам запомнится 2021 год?',
            'Что вы попросите в подарок у Деда Мороза?',
            'И последний вопрос. Хотите пожелать себе что-нибудь в будущее? Вселенная слушает.',
            'Спасибо, вопросы закончились. Если хотите что-то еще добавить - я слушаю.',
        ]

        question_index = 1
        for question in questions:
            assert r.scenario_stages() == {'run'}
            assert r.run_response.ResponseBody.Layout.OutputSpeech == question

            server_directives = r.run_response.ResponseBody.ServerDirectives
            assert any([server_directive.HasField("PatchAsrOptionsForNextRequestDirective") for server_directive in server_directives])

            r = alice(voice(str(question_index)))
            test_responses.add(r)

            server_directives = r.run_response.ResponseBody.ServerDirectives
            assert any([server_directive.HasField("SaveUserAudioDirective") for server_directive in server_directives])

            for server_directive in server_directives:
                if server_directive.HasField("SaveUserAudioDirective"):
                    save_user_audio_directive = server_directives[0].SaveUserAudioDirective
                    assert save_user_audio_directive.StorageOptions.S3Storage.Bucket == "alice-time-capsule"
                    assert save_user_audio_directive.StorageOptions.S3Storage.Path == \
                            f"1083813279/NewYear2021/00000000-00000000-00000000-00000000/{question_index}.opus"

            question_index += 1

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == \
            'Хорошо. Сохраняем капсулу?'
        r = alice(voice('да'))
        test_responses.add(r)

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == \
            'Напоминаю, что, если мы сейчас сохраним капсулу, следующую вы сможете создать только через год. Сохраняем?'
        r = alice(voice('да'))
        test_responses.add(r)

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == \
            '<speaker audio="time_capsule/pre_save_2021_final.opus"> ' \
            'Хорошо. Ваша капсула сохранена. Вы сможете открыть и прослушать ее через год, я обязательно напомню вам об этом. ' \
            '<speaker audio="time_capsule/post_save_2021_final.opus">'

        return str(test_responses)
