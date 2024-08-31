import { formatTime, formatTimeUnit } from './time';

describe('Helpers', () => {
    it('formatTimeUnit', () => {
        expect(formatTimeUnit(1)).toBe('01');
        expect(formatTimeUnit(9)).toBe('09');
        expect(formatTimeUnit(10)).toBe('10');
        expect(formatTimeUnit(15)).toBe('15');
    });

    it('formatTime', () => {
        expect(formatTime(1)).toBe('00:01');
        expect(formatTime(50)).toBe('00:50');
        expect(formatTime(60)).toBe('01:00');
        expect(formatTime(80)).toBe('01:20');
        expect(formatTime(60 * 2)).toBe('02:00');
        expect(formatTime(60 * 59)).toBe('59:00');
        expect(formatTime(60 * 59 + 59)).toBe('59:59');
        expect(formatTime(60 * 60)).toBe('01:00:00');
        expect(formatTime(60 * 60 + 15)).toBe('01:00:15');
        expect(formatTime(60 * 80)).toBe('01:20:00');
        expect(formatTime(60 * 80 + 9)).toBe('01:20:09');
    });
});
