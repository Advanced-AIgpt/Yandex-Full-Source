import { mainTextColor } from '../constants';
import { TextBlockProps } from '../../helpers/types';

type FontStyleType = Required<Pick<TextBlockProps, 'font_size'|'font_weight'|'line_height'|'text_color'>>;

const basicFont: Required<Pick<FontStyleType, 'text_color'>> = {
    text_color: mainTextColor,
};

const basicRegularFont: Required<Pick<FontStyleType, 'text_color' | 'font_weight'>> = {
    ...basicFont,
    font_weight: 'regular',
};

const basicMediumFont: Required<Pick<FontStyleType, 'text_color' | 'font_weight'>> = {
    ...basicFont,
    font_weight: 'medium',
};

// Данные взяты из библиотеки дизайн-системы https://www.figma.com/file/drmdahlxe26M1DsBxkkC22/

// 22/28
export const title22m: FontStyleType = {
    ...basicMediumFont,
    font_size: 22,
    line_height: 28,
};

// 28/36
export const text28r: FontStyleType = {
    ...basicRegularFont,
    font_size: 28,
    line_height: 36,
};

// 28/36
export const text28m: FontStyleType = {
    ...basicMediumFont,
    font_size: 28,
    line_height: 36,
};

// 32/40
export const text32r : FontStyleType = {
    ...basicRegularFont,
    font_size: 32,
    line_height: 40,
};

// 32/40
export const title32m : FontStyleType = {
    ...basicMediumFont,
    font_size: 32,
    line_height: 40,
};

// 34/48
export const text34r : FontStyleType = {
    ...basicRegularFont,
    font_size: 34,
    line_height: 48,
};

// 36/44
export const text36r : FontStyleType = {
    ...basicRegularFont,
    font_size: 36,
    line_height: 44,
};

// 36/46
export const title36m : FontStyleType = {
    ...basicMediumFont,
    font_size: 36,
    line_height: 46,
};

// 40/56
export const text40r : FontStyleType = {
    ...basicRegularFont,
    font_size: 40,
    line_height: 56,
};

// 40/56
export const text40m : FontStyleType = {
    ...basicMediumFont,
    font_size: 40,
    line_height: 56,
};

// 42/52
export const text42r : FontStyleType = {
    ...basicRegularFont,
    font_size: 42,
    line_height: 52,
};

// 42/52
export const text42m : FontStyleType = {
    ...basicMediumFont,
    font_size: 42,
    line_height: 52,
};

// 44/52
export const title44r : FontStyleType = {
    ...basicRegularFont,
    font_size: 44,
    line_height: 52,
};

// 44/52
export const title44m : FontStyleType = {
    ...basicMediumFont,
    font_size: 44,
    line_height: 52,
};

// 48/58
export const title48r : FontStyleType = {
    ...basicRegularFont,
    font_size: 48,
    line_height: 58,
};

// 48/64
export const title48m : FontStyleType = {
    ...basicMediumFont,
    font_size: 48,
    line_height: 64,
};

// 50/62
export const title50r : FontStyleType = {
    ...basicRegularFont,
    font_size: 50,
    line_height: 62,
};

// 54/68
export const title54m : FontStyleType = {
    ...basicMediumFont,
    font_size: 54,
    line_height: 68,
};

// 56/66
export const title56r : FontStyleType = {
    ...basicRegularFont,
    font_size: 56,
    line_height: 66,
};

// 56/66
export const title56m : FontStyleType = {
    ...basicMediumFont,
    font_size: 56,
    line_height: 66,
};

// 60/72
export const title60m : FontStyleType = {
    ...basicMediumFont,
    font_size: 60,
    line_height: 72,
};

// 64/76
export const title64m : FontStyleType = {
    ...basicMediumFont,
    font_size: 64,
    line_height: 76,
};

// 72/88
export const title72m: FontStyleType = {
    ...basicMediumFont,
    font_size: 72,
    line_height: 88,
};

// 72/88
export const title72r: FontStyleType = {
    ...basicRegularFont,
    font_size: 72,
    line_height: 88,
};

// 88/101
export const title88m: FontStyleType = {
    ...basicMediumFont,
    font_size: 88,
    line_height: 101,
};

// 90/146
export const title90m: FontStyleType = {
    ...basicMediumFont,
    font_size: 90,
    line_height: 146,
};

// 130/130
export const title130m: FontStyleType = {
    ...basicMediumFont,
    font_size: 130,
    line_height: 130,
};

// 146/146
export const title146m: FontStyleType = {
    ...basicMediumFont,
    font_size: 146,
    line_height: 146,
};

// 164/164
export const title164m: FontStyleType = {
    ...basicMediumFont,
    font_size: 164,
    line_height: 164,
};

// 240/240
export const title240m: FontStyleType = {
    ...basicMediumFont,
    font_size: 240,
    line_height: 240,
};
