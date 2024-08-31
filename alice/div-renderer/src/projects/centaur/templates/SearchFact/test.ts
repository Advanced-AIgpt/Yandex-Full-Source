import { NAlice } from '../../../../protos';
import SearchFactOld from './SearchFactOld';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { MMRequest } from '../../../../common/helpers/MMRequest';

jest.mock('../../../../common/logger');

describe('Facts scenario', () => {
    it('matches snapshot', () => {
        const mmRequest = new MMRequest({}, {}, {});

        expect(
            AnonymizeDataForSnapshot(SearchFactOld(
                {
                    Text: '69 лет',
                    SearchUrl: 'https://yandex.ru/search/touch/?l10n=ru-RU&lr=213&query_source=alice&text=%D1%81%D0%BA%D0%BE%D0%BB%D1%8C%D0%BA%D0%BE%20%D0%BB%D0%B5%D1%82%20%D0%BF%D1%83%D1%82%D0%B8%D0%BD%D1%83',
                    Title: 'Возраст',
                    SerpData: {},
                    Hostname: 'https://wikipedia.org',
                },
                mmRequest,
            )),
        ).toMatchSnapshot();
    });

    it('matches snapshot with utterance', () => {
        const mmRequestInputWithTextReq = new NAlice.NScenarios.TInput;
        mmRequestInputWithTextReq.Voice = new NAlice.NScenarios.TInput.TVoice();
        mmRequestInputWithTextReq.Voice.Utterance = 'Сколько лет Путину?';
        const mmRequest = new MMRequest({}, mmRequestInputWithTextReq, {});

        expect(AnonymizeDataForSnapshot(SearchFactOld({
            Text: '',
            SearchUrl: 'https://yandex.ru/search/touch/?l10n=ru-RU&lr=213&query_source=alice&text=%D1%81%D0%BA%D0%BE%D0%BB%D1%8C%D0%BA%D0%BE%20%D0%BB%D0%B5%D1%82%20%D0%BF%D1%83%D1%82%D0%B8%D0%BD%D1%83',
            Title: 'Возраст',
            SerpData: {},
            Hostname: 'https://wikipedia.org',
        }, mmRequest))).toMatchSnapshot();
    });
});
