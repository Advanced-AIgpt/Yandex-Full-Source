import ExampleDiv from './ExampleDiv';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { createRequestState } from '../../../../registries/common';
import { MMRequest } from '../../../../common/helpers/MMRequest';

jest.mock('../../../../common/logger');

describe('example', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(ExampleDiv({
            hello: 'world',
        }, new MMRequest({}, {}, {}), createRequestState()))).toMatchSnapshot();
    });
});
