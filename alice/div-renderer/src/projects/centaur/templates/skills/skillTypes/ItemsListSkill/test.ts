import skills from '../../index';
import { params } from '../../stabs/ItemsListData';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';

jest.mock('../../../../../../common/logger');

describe('ItemsListSkill', () => {
    it('should equal snapshot', () => {
        expect(AnonymizeDataForSnapshot(skills(params))).toMatchSnapshot();
    });
});
