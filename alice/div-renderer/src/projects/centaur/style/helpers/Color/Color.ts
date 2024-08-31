/* eslint-disable no-magic-numbers */
function componentToHex(c: number) {
    const hex = c.toString(16);
    return hex.length === 1 ? '0' + hex : hex;
}

/**
 * Класс предназначен для удобной генерации констант цвета. НЕ использовать в других местах
 */
export class Color {
    private color: {
        r: number;
        g: number;
        b: number;
        a: number;
    };

    constructor(color: string) {
        const colorToSave = Color.hexToRgb(color);
        if (!colorToSave) {
            throw new TypeError('Constructor argument is not a color');
        }
        this.color = colorToSave;
    }

    setOpacity(percent: number): this {
        if (percent < 0 || percent > 100) {
            throw new TypeError('The percentage should be between 0 and 100');
        }

        this.color.a = Math.round(255 * percent / 100);
        return this;
    }

    /**
     * Изменить яркость.
     * Переводит `rgb` в `hsb`, и изменяет `b`
     * @param brightnessDelta
     * @returns {this}
     */
    changeBrightness(brightnessDelta: number): this {
        const {
            h,
            s,
            v,
        } = Color.rgbToHsb(this.color);

        this.color = {
            ...Color.hsbToRgb({
                h,
                s,
                v: v + brightnessDelta,
            }),
            a: this.color.a,
        };

        return this;
    }

    toString(): string {
        let compA = this.color.a < 255 ? componentToHex(this.color.a).toString() : '';
        let compR = componentToHex(this.color.r);
        let compG = componentToHex(this.color.g);
        let compB = componentToHex(this.color.b);

        if (
            (compA.length === 0 || compA[0] === compA[1]) &&
            compR[0] === compR[1] &&
            compG[0] === compG[1] &&
            compB[0] === compB[1]
        ) {
            compA = compA.length > 0 ? compA[0] : '';
            compR = compR[0];
            compG = compG[0];
            compB = compB[0];
        }

        return `#${compA}${compR}${compG}${compB}`;
    }

    static hexToRgb(hex: string) {
        hex = hex.toLowerCase().substring(1);
        if (hex.length === 3) {
            hex = 'ff' + hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
        }
        if (hex.length === 4) {
            hex = hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2] + hex[3] + hex[3];
        }
        if (hex.length === 6) {
            hex = 'ff' + hex;
        }

        const result = /^([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
        return result ? {
            r: parseInt(result[2], 16),
            g: parseInt(result[3], 16),
            b: parseInt(result[4], 16),
            a: parseInt(result[1], 16),
        } : null;
    }

    static rgbToHsb({ r, g, b }: {r: number; g: number; b: number;}) {
        const rabs = r / 255;
        const gabs = g / 255;
        const babs = b / 255;
        let rr: number;
        let gg: number;
        let bb: number;
        let h = 0;
        let s: number;
        const v = Math.max(rabs, gabs, babs);
        const diff = v - Math.min(rabs, gabs, babs);
        const diffc = (c: number) => (v - c) / 6 / diff + 1 / 2;
        if (diff === 0) {
            h = s = 0;
        } else {
            s = diff / v;
            rr = diffc(rabs);
            gg = diffc(gabs);
            bb = diffc(babs);
            if (rabs === v) {
                h = bb - gg;
            } else if (gabs === v) {
                h = (1 / 3) + rr - bb;
            } else if (babs === v) {
                h = (2 / 3) + gg - rr;
            }
            if (h < 0) {
                h += 1;
            } else if (h > 1) {
                h -= 1;
            }
        }
        return {
            h: Math.round(h * 360),
            s: Math.round(s * 100),
            v: Math.round(v * 100),
        };
    }

    static hsbToRgb({ h, s, v }: { h: number; s: number; v: number; }) {
        s = s / 100;
        v = v / 100;
        const hprime = h / 60;
        const c = v * s;
        const x = c * (1 - Math.abs(hprime % 2 - 1));
        const m = v - c;
        let r = 0; let g = 0; let b = 0;
        if (!hprime) {
            r = 0; g = 0; b = 0;
        }
        if (hprime >= 0 && hprime < 1) {
            r = c; g = x; b = 0;
        }
        if (hprime >= 1 && hprime < 2) {
            r = x; g = c; b = 0;
        }
        if (hprime >= 2 && hprime < 3) {
            r = 0; g = c; b = x;
        }
        if (hprime >= 3 && hprime < 4) {
            r = 0; g = x; b = c;
        }
        if (hprime >= 4 && hprime < 5) {
            r = x; g = 0; b = c;
        }
        if (hprime >= 5 && hprime < 6) {
            r = c; g = 0; b = x;
        }

        r = Math.round((r + m) * 255);
        g = Math.round((g + m) * 255);
        b = Math.round((b + m) * 255);

        return { r, g, b };
    }

    static diffBetween(color1: Color, color2: Color) {
        return Math.sqrt(
            Math.pow(color1.color.r - color2.color.r, 2) +
            Math.pow(color1.color.g - color2.color.g, 2) +
            Math.pow(color1.color.b - color2.color.b, 2),
        );
    }

    static getClosestIndex(color: Color, to: Color[]) {
        let i = 0;
        let closest = 0;
        let closestValueD = Color.diffBetween(to[closest], color);

        do {
            i++;
            const d = Color.diffBetween(to[i], color);
            if (d < closestValueD) {
                closest = i;
                closestValueD = d;
                if (closestValueD === 0) {
                    break;
                }
            }
        } while (i < to.length - 1);

        return closest;
    }
}
