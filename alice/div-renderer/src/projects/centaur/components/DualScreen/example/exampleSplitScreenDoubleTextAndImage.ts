import { DualScreen } from '../DualScreen';
import PartSimpleText from '../partComponents/PartSimpleText';
import getColorSet from '../../../style/colorSet';
import { getStaticS3Asset } from '../../../helpers/assets';

export default function() {
    const colorSet = getColorSet();

    return DualScreen({
        firstDiv: PartSimpleText({
            colorSet,
            text: 'text',
            subText: 'subText',
            source: {
                name: 'source.name',
            },
        }),
        secondDiv: PartSimpleText({
            colorSet,
            text: 'text',
            subText: 'subText',
            source: {
                name: 'source.name',
            },
            backgroundImage: getStaticS3Asset('discovery/anecdote.png'),
            backgroundColor: colorSet.mainColorOpacity60,
        }),
        colorSet,
    });
}
