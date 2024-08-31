import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import screenSaverRender from './screenSaver';
import { MMRequest } from '../../../../common/helpers/MMRequest';

jest.mock('../../../../common/logger');

describe('Photoframe scenario', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(screenSaverRender({
            ImageUrl: 'https://dialogs.s3.yandex.net/smart_displays/photoframe/nature29.png',
        }, new MMRequest({}, {}, {})))).toMatchSnapshot();
    });
});
