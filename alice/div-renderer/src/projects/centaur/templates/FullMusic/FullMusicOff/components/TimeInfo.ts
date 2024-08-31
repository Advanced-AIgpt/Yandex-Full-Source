import { ContainerBlock, TextBlock, WrapContentSize } from 'divcard2';
import { title32m } from '../../../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../../../style/constants';

export function TimeInfo() {
    return new ContainerBlock({
        orientation: 'horizontal',
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        alignment_vertical: 'center',
        margins: {
            left: 20,
        },
        paddings: {
            bottom: 2,
        },
        items: [
            new TextBlock({
                ...title32m,
                text: '00:00',
                text_color: colorWhiteOpacity50,
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                extensions: [
                    {
                        id: 'music-player-progress',
                    },
                ],
            }),
            new TextBlock({
                ...title32m,
                text: ' / ',
                text_color: colorWhiteOpacity50,
                width: new WrapContentSize(),
                height: new WrapContentSize(),
            }),
            new TextBlock({
                ...title32m,
                text: '00:00',
                text_color: colorWhiteOpacity50,
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                extensions: [
                    {
                        id: 'music-player-duration',
                    },
                ],
            }),
        ],
    });
}
