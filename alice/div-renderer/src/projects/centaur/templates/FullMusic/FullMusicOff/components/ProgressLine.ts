import { ContainerBlock, MatchParentSize, WrapContentSize } from 'divcard2';
import { offsetFromEdgeOfScreen } from '../../../../style/constants';
import { TimeInfo } from './TimeInfo';
import MusicProgressDivCustom from '../../../../divCustoms/MusicProgress';

export function ProgressLine() {
    return new ContainerBlock({
        orientation: 'horizontal',
        width: new MatchParentSize(),
        height: new WrapContentSize(),
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
            top: 16,
        },
        items: [
            MusicProgressDivCustom(),
            TimeInfo(),
        ],
    });
}
