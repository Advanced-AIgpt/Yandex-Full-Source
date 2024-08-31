import { Div, IDivAction } from 'divcard2';
import CloseButton from '../CloseButton';
import { Layer } from '../../common/layers';
import TopLayout from '../TopLayout';

interface ICloseButtonWrapperProps {
    div: Div;
    layer?: Layer;
    actions?: IDivAction[];
    preventDefault?: boolean;
    closeButtonProps?: Parameters<typeof CloseButton>[0];
}

export function CloseButtonWrapper({
    div,
    layer = Layer.DIALOG,
    actions = [],
    preventDefault,
    closeButtonProps,
}: ICloseButtonWrapperProps) {
    return TopLayout({
        rightTop: [
            CloseButton({
                ...closeButtonProps,
                options: {
                    margins: {
                        top: 36,
                        right: 36,
                    },
                    actions,
                    ...closeButtonProps?.options,
                },
                layer,
                preventDefault,
            }),
        ],
        content: div,
    });
}
