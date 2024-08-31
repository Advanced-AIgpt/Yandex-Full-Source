import { sortTimeOfDay } from './common';

describe('Traffic card main screen common', () => {
    it('sort by time of day', () => {
        expect(sortTimeOfDay([
            {
                time: 0,
                trafficValue: 1,
            },
            {
                time: 1,
                trafficValue: 1,
            },
            {
                time: 2,
                trafficValue: 1,
            },
            {
                time: 3,
                trafficValue: 1,
            },
            {
                time: 4,
                trafficValue: 1,
            },
        ])).toEqual([
            {
                time: 0,
                trafficValue: 1,
            },
            {
                time: 1,
                trafficValue: 1,
            },
            {
                time: 2,
                trafficValue: 1,
            },
            {
                time: 3,
                trafficValue: 1,
            },
            {
                time: 4,
                trafficValue: 1,
            },
        ]);

        expect(sortTimeOfDay([
            {
                time: 19,
                trafficValue: 1,
            },
            {
                time: 20,
                trafficValue: 1,
            },
            {
                time: 21,
                trafficValue: 1,
            },
            {
                time: 22,
                trafficValue: 1,
            },
            {
                time: 23,
                trafficValue: 1,
            },
        ])).toEqual([
            {
                time: 19,
                trafficValue: 1,
            },
            {
                time: 20,
                trafficValue: 1,
            },
            {
                time: 21,
                trafficValue: 1,
            },
            {
                time: 22,
                trafficValue: 1,
            },
            {
                time: 23,
                trafficValue: 1,
            },
        ]);

        expect(sortTimeOfDay([
            {
                time: 0,
                trafficValue: 1,
            },
            {
                time: 1,
                trafficValue: 1,
            },
            {
                time: 21,
                trafficValue: 1,
            },
            {
                time: 22,
                trafficValue: 1,
            },
            {
                time: 23,
                trafficValue: 1,
            },
        ])).toEqual([
            {
                time: 21,
                trafficValue: 1,
            },
            {
                time: 22,
                trafficValue: 1,
            },
            {
                time: 23,
                trafficValue: 1,
            },
            {
                time: 0,
                trafficValue: 1,
            },
            {
                time: 1,
                trafficValue: 1,
            },
        ]);

        expect(sortTimeOfDay([
            {
                time: null,
                trafficValue: 1,
            },
            {
                time: 1,
                trafficValue: 1,
            },
            {
                time: 22,
                trafficValue: 1,
            },
            {
                time: 23,
                trafficValue: 1,
            },
        ])).toEqual([
            {
                time: 22,
                trafficValue: 1,
            },
            {
                time: 23,
                trafficValue: 1,
            },
            {
                time: 1,
                trafficValue: 1,
            },
        ]);
    });
});
