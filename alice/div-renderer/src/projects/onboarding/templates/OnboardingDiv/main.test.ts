import OnboardingDiv from './OnboardingDiv';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { createRequestState } from '../../../../registries/common';
import { MMRequest } from '../../../../common/helpers/MMRequest';

jest.mock('../../../../common/logger');

describe('onboarding', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(OnboardingDiv({
            Buttons: [
                {
                    Title: 'Что это играет?',
                    ActionId: 'div_action_0',
                },
                {
                    Title: 'Погода',
                    ActionId: 'div_action_1',
                },
                {
                    Title: 'Игра «День Выборов»',
                    ActionId: 'div_action_2',
                },
            ],
        }, new MMRequest({}, {}, {}), createRequestState()))).toMatchSnapshot();
    });
});
