import skills from '../../index';
import { params } from '../../stabs/TextSkillData';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';

jest.mock('../../../../../../common/logger');

describe('TextSkill', () => {
    it('should equal snapshot', () => {
        expect(AnonymizeDataForSnapshot(skills(params))).toMatchSnapshot();
    });
});
