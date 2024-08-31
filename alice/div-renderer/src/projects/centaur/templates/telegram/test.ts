import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import ChooseContact from './ChooseContact';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { createRequestState } from '../../../../registries/common';

jest.mock('../../../../common/logger');

describe('Telegram ChooseContact Scene', () => {
    it('matches snapshot', () => {
        const mmRequest = new MMRequest({}, {}, {});

        expect(AnonymizeDataForSnapshot(ChooseContact({
            ContactData: [{
                TelegramContactData: {
                    DisplayName: 'Name',
                    UserId: 'UserId',
                },
            }],
        }, mmRequest, createRequestState(mmRequest)))).toMatchSnapshot();
    });
});
