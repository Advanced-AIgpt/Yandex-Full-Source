import { getPalette } from '../../../centaur/helpers/palette';
import { Color } from '../../../centaur/style/helpers/Color/Color';

export const greetingsPalette = getPalette({
    scope: 'greetings',
    extendsCommon: true,
    colorScheme: {
        border: {
            light: '#e6e1f5',
            dark: new Color('#7a55ff').setOpacity(40).toString(),
        },
    },
});
