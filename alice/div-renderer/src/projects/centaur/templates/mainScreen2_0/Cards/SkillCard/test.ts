import SkillsCard from './SkillsCard';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';
import { NAlice } from '../../../../../../protos';
import { getSkillCardData } from './getData';
import { logger } from '../../../../../../common/logger';
const TCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.TCardData;

jest.mock('../../../../../../common/logger');

describe('Main screen empty card', () => {
    it('should get actual data', () => {
        expect(getSkillCardData(new TCardData({}))).toBeNull();

        expect(getSkillCardData(new TCardData({
            YouTubeCardData: {},
        }))).toBeNull();

        expect(getSkillCardData(new TCardData({
            YouTubeCardData: {},
            ExternalSkillCardData: {
            },
        }))).not.toBeNull();

        expect(getSkillCardData(new TCardData({
            ExternalSkillCardData: {
                skillInfo: {
                },
                widgetGalleryData: {},
            },
        }))).not.toBeNull();
    });

    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(SkillsCard({
            type: 'skill',
            title: 'Медуза',
            comment: 'Tesla начала устанавливать терминалы Starlink на зарядных станциях Supercharger',
            skillName: 'Навык',
            skillImage: '',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalledTimes(0);
    });
});
