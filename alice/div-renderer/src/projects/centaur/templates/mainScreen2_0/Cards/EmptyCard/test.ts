import EmptyCard from './EmptyCard';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';

jest.mock('../../../../../../common/logger');

describe('Main screen empty card', () => {
    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(EmptyCard({
            type: 'empty',
        }))).toMatchSnapshot();
    });
});
