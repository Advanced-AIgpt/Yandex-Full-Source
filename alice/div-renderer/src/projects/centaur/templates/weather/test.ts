import { weatherRenderDay, weatherRenderDayHours, weatherRenderPartDay, weatherRenderDaysRange } from './scenario';
import * as dayJson from '../../../../dev/samples/weather/dayData.json';
import * as dayHoursJson from '../../../../dev/samples/weather/dayHours.json';
import * as dayPartJson from '../../../../dev/samples/weather/dayPart.json';
import * as daysRangeJson from '../../../../dev/samples/weather/daysRange.json';
import { weatherRenderTeaser } from './teaser';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { MMRequest } from '../../../../common/helpers/MMRequest';

jest.mock('../../../../common/logger');

const mmRequest = new MMRequest({}, {}, {});

describe('Weather main', () => {
    it('matches snapshot Weather day', () => {
        expect(AnonymizeDataForSnapshot(weatherRenderDay(dayJson, mmRequest))).toMatchSnapshot();
    });

    it('matches snapshot Weather day hours', () => {
        expect(AnonymizeDataForSnapshot(weatherRenderDayHours(dayHoursJson, mmRequest))).toMatchSnapshot();
    });

    it('matches snapshot Weather day part', () => {
        expect(AnonymizeDataForSnapshot(weatherRenderPartDay(dayPartJson, mmRequest))).toMatchSnapshot();
    });

    it('matches snapshot Weather days range', () => {
        expect(AnonymizeDataForSnapshot(weatherRenderDaysRange(daysRangeJson, mmRequest))).toMatchSnapshot();
    });
});

describe('Weather teaser', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(weatherRenderTeaser(dayHoursJson, mmRequest))).toMatchSnapshot();
    });
});
