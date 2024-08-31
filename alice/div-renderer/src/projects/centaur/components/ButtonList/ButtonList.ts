import { ContainerBlock, Div, GalleryBlock, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { title44r } from '../../style/Text/Text';
import { colorMoreThenBlack, colorWhite, offsetFromEdgeOfScreen } from '../../style/constants';

interface buttonData {
    text: string,
    url: string,
    payload: null | string,
}

interface Props {
    buttons: buttonData[];
}

function getButton(button: buttonData): Div {
    return new ContainerBlock({
        orientation: 'horizontal',
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        paddings: {
            left: 40,
            top: 29,
            right: 40,
            bottom: 29,
        },
        background: [
            new SolidBackground({ color: colorWhite }),
        ],
        border: {
            corner_radius: 28,
        },
        items: [
            new TextBlock({
                ...title44r,
                text_color: colorMoreThenBlack,
                text: button.text,
                action: {
                    url: button.url,
                    log_id: 'button_list_button',
                },
            }),
        ],
    });
}

export function ButtonList({ buttons }: Props): Div {
    return new GalleryBlock({
        orientation: 'horizontal',
        item_spacing: 24,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        margins: {
            top: 48,
        },
        items: buttons.map(getButton),
    });
}
