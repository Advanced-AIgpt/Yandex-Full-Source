import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { getTemperatureString } from './weather';

describe('Weather helpers', () => {
    it('Check formatted weather string', () => {
        expect(AnonymizeDataForSnapshot(getTemperatureString(0))).toBe('0°');
        expect(AnonymizeDataForSnapshot(getTemperatureString(3))).toBe('+3°');
        expect(AnonymizeDataForSnapshot(getTemperatureString(-4))).toBe('–4°');
    });
});
