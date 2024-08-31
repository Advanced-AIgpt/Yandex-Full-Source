import { Color } from './Color';

describe('Color object', () => {
    it('should crash on invalid input', () => {
        expect(() => new Color('#zxc')).toThrow(TypeError);
        expect(() => new Color('fff')).toThrow(TypeError);
        expect(() => new Color('#-12343')).toThrow(TypeError);
    });

    it('should color object get all types of colors', () => {
        expect((new Color('#fff')).toString()).toBe('#fff');
        expect((new Color('#0fff')).toString()).toBe('#0fff');
        expect((new Color('#9fca')).toString()).toBe('#9fca');
        expect((new Color('#ffffccaa')).toString()).toBe('#fca');
        expect((new Color('#f1c2a3')).toString()).toBe('#f1c2a3');
        expect((new Color('#c4f1c2a3')).toString()).toBe('#c4f1c2a3');
    });

    it('should correctly set opacity', () => {
        expect((new Color('#c4f1c2a3')).setOpacity(0).toString()).toBe('#00f1c2a3');
        expect((new Color('#c4f1c2a3')).setOpacity(100).toString()).toBe('#f1c2a3');
        expect((new Color('#c4f1c2a3')).setOpacity(90).toString()).toBe('#e6f1c2a3');
        expect(() => (new Color('#c4f1c2a3')).setOpacity(101).toString()).toThrow(TypeError);
    });

    it('should correctly set rgb to hsb', () => {
        expect(Color.rgbToHsb({
            r: 43,
            g: 11,
            b: 6,
        })).toEqual({
            h: 8,
            s: 86,
            v: 17,
        });
    });

    it('should correctly set hsb to rgb', () => {
        expect(Color.hsbToRgb({
            h: 8,
            s: 86,
            v: 17,
        })).toEqual({
            r: 43,
            g: 11,
            b: 6,
        });
    });

    it('should correctly set brightness', () => {
        expect(new Color('#2B0B06').changeBrightness(8).toString()).toBe('#401009');
        expect(new Color('#401009').changeBrightness(8).toString()).toBe('#54150c');
        expect(new Color('#400f09').changeBrightness(8).changeBrightness(-8).toString()).toBe('#400f09');
    });
});
