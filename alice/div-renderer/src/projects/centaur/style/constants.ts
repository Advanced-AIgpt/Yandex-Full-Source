/* Offsets */
import { DivBackground, GradientBackground, IDivAnimation, SolidBackground } from 'divcard2';
import { Color } from './helpers/Color/Color';

export const offsetFromEdgeOfScreen = 48;
export const offsetTopWithSearchItem = 142;
export const offsetForScaleAnimation = Math.floor(offsetFromEdgeOfScreen * 0.4);

export const offsetFromEdgeOfPartOfScreen = 36;

/* Colors */
export const colorWhite = '#fff';
export const colorWhiteOpacity0 = (new Color(colorWhite)).setOpacity(0).toString();
export const colorWhiteOpacity7 = (new Color(colorWhite)).setOpacity(7).toString();
export const colorWhiteOpacity10 = (new Color(colorWhite)).setOpacity(10).toString();
export const colorWhiteOpacity15 = (new Color(colorWhite)).setOpacity(15).toString();
export const colorWhiteOpacity40 = (new Color(colorWhite)).setOpacity(40).toString();
export const colorWhiteOpacity50 = (new Color(colorWhite)).setOpacity(50).toString();
export const colorWhiteOpacity53 = (new Color(colorWhite)).setOpacity(53).toString();
export const colorWhiteOpacity60 = (new Color(colorWhite)).setOpacity(60).toString();
export const colorWhiteOpacity90 = (new Color(colorWhite)).setOpacity(90).toString();
export const colorLightGrey = '#ccc';
export const colorGrey = '#717173';
export const colorDanger = '#FA5846';
export const colorDarkGrey = '#2A2B2D';
export const colorMoreThenBlack = '#121316';
export const colorMoreThenBlackOpacity48 = (new Color(colorMoreThenBlack)).setOpacity(48).toString();
export const colorDarkGrey2 = '#292A2D';
export const colorDarkGrey1Opacity70 = (new Color(colorMoreThenBlack)).setOpacity(70).toString();
export const colorBlack = '#000';
export const colorBlackOpacity90 = (new Color(colorBlack)).setOpacity(90).toString();
export const colorBlackOpacity40 = (new Color(colorBlack)).setOpacity(40).toString();
export const colorBlackOpacity70 = (new Color(colorBlack)).setOpacity(70).toString();
export const colorBlackOpacity0 = (new Color(colorBlack)).setOpacity(0).toString();
export const colorPurple = '#6934ff';
export const errorStatusColor = '#f76d5e';
export const colorReadButton = (new Color(colorBlack)).setOpacity(20).toString();

/* Theme Colors */
export const mainTextColor = colorWhite;
export const disabledTextColor = (new Color('#8a8b8c')).setOpacity(50).toString();
export const disabledImageColor = (new Color('#000')).setOpacity(50).toString();
export const mainBackground = colorMoreThenBlack;
export const mainBackgroundOpacity0 = (new Color(mainBackground)).setOpacity(0).toString();
export const paginationActiveColor = colorWhite;
export const paginationNotActiveColor = colorDarkGrey;
export const skillNameColorTitle = '#88898A';

export const colorBlueText = '#e6eef9';
export const colorBlueTextOpacity50 = (new Color(colorBlueText)).setOpacity(50).toString();
export const colorBlueTextOpacity10 = (new Color(colorBlueText)).setOpacity(10).toString();

export const transparentSuggestColor = colorWhiteOpacity10;
export const notTransparentSuggestColor = colorDarkGrey2;

/* Background presets */
type backgroundPreset = DivBackground[];

export const simpleBackground: backgroundPreset = [
    new SolidBackground({ color: mainBackground }),
];

const colorMiddleBlackAndBackground = (new Color('#050607')).setOpacity(62.62).toString();

function generateGradient(colors: string[], angle: number) {
    return {
        angle,
        colors,
    };
}

function generateGradientToBlackOptions(angle: number) {
    return generateGradient([
        colorBlackOpacity90,
        colorMiddleBlackAndBackground,
        mainBackgroundOpacity0,
    ], angle);
}

export const gradientToBlackTop: backgroundPreset = [
    new GradientBackground(generateGradientToBlackOptions(270)),
];

export const gradientToBlackBottom: backgroundPreset = [
    new GradientBackground(generateGradientToBlackOptions(90)),
];

export const backgroundToImageWithText: backgroundPreset = [
    new SolidBackground({
        color: colorMoreThenBlackOpacity48,
    }),
];

export const debugBackground1: backgroundPreset = [
    new SolidBackground({ color: '#f00' }),
];

export const debugBackground2: backgroundPreset = [
    new SolidBackground({ color: '#ff0' }),
];

export const whiteBackgroundOpacity10: backgroundPreset = [
    new SolidBackground({ color: colorWhiteOpacity10 }),
];

/* animations */
const animationTime = 300; // 0.3 sec

const tappableIn: IDivAnimation = {
    name: 'set',
    items: [{
        duration: animationTime,
        interpolator: 'ease_in_out',
        name: 'scale',
        start_value: 1.1,
        end_value: 1,
        start_delay: animationTime,
    }, {
        duration: animationTime,
        interpolator: 'ease_in_out',
        name: 'fade',
        start_value: 0.3,
        end_value: 1,
        start_delay: animationTime,
    }],
};
const tappableOut: IDivAnimation = {
    name: 'set',
    items: [{
        duration: animationTime,
        interpolator: 'ease_in_out',
        name: 'scale',
        start_value: 1,
        end_value: 1.1,
    }, {
        duration: animationTime,
        interpolator: 'ease_in_out',
        name: 'fade',
        start_value: 1,
        end_value: 0.3,
    }],
};

export const tappableAnimation = {
    animation_in: tappableIn,
    animation_out: tappableOut,
};

export const INTERFACE_SCALE = 1;

/**
 * @deprecated
 * @param value
 * @returns {Number}
 */
export function scalableValue(value: number) {
    return Math.floor(value * INTERFACE_SCALE);
}
