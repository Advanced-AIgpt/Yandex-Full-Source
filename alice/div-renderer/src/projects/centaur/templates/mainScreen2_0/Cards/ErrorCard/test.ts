import ErrorCard from './ErrorCard';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';

describe('Main screen empty card', () => {
    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(ErrorCard({ colIndex: 0, rowIndex: 0 }))).toMatchSnapshot();
    });
});
