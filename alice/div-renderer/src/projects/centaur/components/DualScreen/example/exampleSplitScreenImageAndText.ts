import { DualScreen } from '../DualScreen';
import PartSimpleText from '../partComponents/PartSimpleText';
import getColorSet from '../../../style/colorSet';
import PartImage from '../partComponents/PartImage';
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
        secondDiv: PartImage({
            imageUrl: getStaticS3Asset('discovery/anecdote.png'),
        }),
        colorSet,
    });
}
