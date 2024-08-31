import { yaMapsRatingColorMap, getYaMapsRatingBackgroundColor, formatRatingValue } from './YaMapsRating';

describe('YaMapsRating', () => {
    it('getYaMapsRatingBackgroundColor', () => {
        expect(getYaMapsRatingBackgroundColor(6)).toEqual(yaMapsRatingColorMap.best);

        expect(getYaMapsRatingBackgroundColor(5)).toEqual(yaMapsRatingColorMap.best);
        expect(getYaMapsRatingBackgroundColor(4.9)).toEqual(yaMapsRatingColorMap.best);
        expect(getYaMapsRatingBackgroundColor(4.4)).toEqual(yaMapsRatingColorMap.best);
        expect(getYaMapsRatingBackgroundColor(4)).toEqual(yaMapsRatingColorMap.best);

        expect(getYaMapsRatingBackgroundColor(3.9)).toEqual(yaMapsRatingColorMap.good);
        expect(getYaMapsRatingBackgroundColor(3.4)).toEqual(yaMapsRatingColorMap.good);
        expect(getYaMapsRatingBackgroundColor(3)).toEqual(yaMapsRatingColorMap.good);

        expect(getYaMapsRatingBackgroundColor(2.9)).toEqual(yaMapsRatingColorMap.notGood);
        expect(getYaMapsRatingBackgroundColor(2.4)).toEqual(yaMapsRatingColorMap.notGood);
        expect(getYaMapsRatingBackgroundColor(2)).toEqual(yaMapsRatingColorMap.notGood);
        expect(getYaMapsRatingBackgroundColor(1.9)).toEqual(yaMapsRatingColorMap.notGood);
        expect(getYaMapsRatingBackgroundColor(1.4)).toEqual(yaMapsRatingColorMap.notGood);
        expect(getYaMapsRatingBackgroundColor(1)).toEqual(yaMapsRatingColorMap.notGood);

        expect(getYaMapsRatingBackgroundColor(0.9)).toEqual(yaMapsRatingColorMap.bad);
        expect(getYaMapsRatingBackgroundColor(0.4)).toEqual(yaMapsRatingColorMap.bad);
        expect(getYaMapsRatingBackgroundColor(0)).toEqual(yaMapsRatingColorMap.bad);

        expect(getYaMapsRatingBackgroundColor(-1)).toEqual(yaMapsRatingColorMap.bad);
    });

    it('formatRatingValue', () => {
        expect(formatRatingValue(5)).toEqual('5,0');
        expect(formatRatingValue(4.9)).toEqual('4,9');
    });
});
