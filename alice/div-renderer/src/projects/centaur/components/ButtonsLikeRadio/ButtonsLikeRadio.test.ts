import ButtonsLikeRadio from './ButtonsLikeRadio';
import { TextBlock } from 'divcard2';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { createRequestState } from '../../../../registries/common';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';

describe('Check ButtonsLikeRadio render', () => {
    it('should success render', () => {
        const mmRequest = new MMRequest({}, {}, {});
        const requestState = createRequestState(mmRequest);

        expect(AnonymizeDataForSnapshot(ButtonsLikeRadio({
            requestState,
            buttonListId: 'id',
            buttons: [
                {
                    activeDiv: actions => new TextBlock({
                        text: 'active',
                        actions,
                    }),
                    inactiveDiv: actions => new TextBlock({
                        text: 'inactive',
                        actions,
                    }),
                },
            ],
        }))).toMatchSnapshot();
    });
});
