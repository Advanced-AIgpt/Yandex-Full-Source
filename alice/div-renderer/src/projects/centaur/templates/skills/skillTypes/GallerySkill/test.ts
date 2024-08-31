import skills from '../../index';
import { params } from '../../stabs/BigImageData';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';

jest.mock('../../../../../../common/logger');

describe('BigImageSkill', () => {
    it('should equal snapshot', () => {
        expect(AnonymizeDataForSnapshot(skills(params))).toMatchSnapshot();
    });
});
