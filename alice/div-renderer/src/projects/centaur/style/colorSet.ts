import { Color } from './helpers/Color/Color';

export type IColor = string;

export interface IColorSet {
    mainColorClass: Color;
    textColor: IColor;
    textColorOpacity50: IColor;
    textColorOpacity10: IColor;
    textColorOpacity20: IColor;
    iconsBgColor: IColor;
    mainColor: IColor;
    mainColorOpacity0: IColor;
    mainColorOpacity60: IColor;
    mainColorOpacity90: IColor;
    mainColor1: IColor;
    mainColor2: IColor;
    suggestsBackground: IColor;
    dimeColor: IColor;
    closeButtonBackgroundColor: IColor;
}

function generateColorSet(mainColor: IColor, secondaryColor: IColor, suggestsBackground?: IColor): IColorSet {
    return {
        mainColorClass: new Color(mainColor),
        textColor: new Color(secondaryColor).toString(),
        textColorOpacity50: new Color(secondaryColor).setOpacity(50).toString(),
        textColorOpacity20: new Color(secondaryColor).setOpacity(20).toString(),
        textColorOpacity10: new Color(secondaryColor).setOpacity(10).toString(),
        iconsBgColor: new Color(secondaryColor).setOpacity(10).toString(),
        mainColor: new Color(mainColor).toString(),
        mainColorOpacity0: new Color(mainColor).setOpacity(0).toString(),
        mainColorOpacity60: new Color(mainColor).setOpacity(60).toString(),
        mainColorOpacity90: new Color(mainColor).setOpacity(90).toString(),
        mainColor1: new Color(mainColor).changeBrightness(8).toString(),
        mainColor2: new Color(mainColor).changeBrightness(16).toString(),
        dimeColor: new Color(mainColor).setOpacity(60).toString(),
        suggestsBackground: suggestsBackground || new Color(mainColor).changeBrightness(24).toString(),
        closeButtonBackgroundColor: new Color(mainColor).changeBrightness(8).toString(),
    };
}

const ColorSet: IColorSet[] = [
    generateColorSet('#1A201B', '#EAF6EC'),
    generateColorSet('#0E1115', '#E6EEF9'),
    generateColorSet('#1C1822', '#F4ECFF'),
    generateColorSet('#211819', '#F8EAEC'),
    generateColorSet('#211E18', '#F2EEE6'),
    generateColorSet('#211B18', '#F6E7DF'),
];

export default function getColorSet({
    id,
    color,
}: {
    id?: string | number;
    color?: string;
} = {}): IColorSet {
    let colorSet: IColorSet;
    if (color) {
        const colorIndex = Color.getClosestIndex(new Color(color), ColorSet.map(el => el.mainColorClass));
        colorSet = ColorSet[colorIndex];
    } else if (id && typeof id === 'string') {
        colorSet = ColorSet[id.length % ColorSet.length];
    } else if (id && typeof id === 'number') {
        colorSet = ColorSet[Math.round(id) % ColorSet.length];
    } else {
        colorSet = ColorSet[0];
    }

    return colorSet;
}

export const boltalkaColorSet = generateColorSet('#3A228D', '#F8F2E5', '#4E3898');
