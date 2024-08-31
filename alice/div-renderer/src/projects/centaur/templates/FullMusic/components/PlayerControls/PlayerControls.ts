import { ContainerBlock, WrapContentSize } from 'divcard2';
import { PreviousPlayerButton } from './components/PreviousPlayerButton';
import { NextPlayerButton } from './components/NextPlayerButton';
import { PlayPausePlayerButton } from './components/PlayPausePlayerButton';
import { IRequestState } from '../../../../../../common/types/common';

export function PlayerControls(requestState: IRequestState) {
    return new ContainerBlock({
        orientation: 'horizontal',
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        items: [
            PreviousPlayerButton(),
            PlayPausePlayerButton({ requestState }),
            NextPlayerButton(),
        ],
    });
}
