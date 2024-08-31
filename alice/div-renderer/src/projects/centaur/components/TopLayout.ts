import { ContainerBlock, Div, MatchParentSize } from 'divcard2';
import { compact } from 'lodash';
import CloseButton from './CloseButton';

type ContainerOptions = Omit<ConstructorParameters<typeof ContainerBlock>[0], 'items'>;
type Items = ConstructorParameters<typeof ContainerBlock>[0]['items'];

interface ITopLayoutProps {
    rightTop?: Items;
    rightTopOptions?: ContainerOptions;
    bottomLine?: Items;
    bottomLineOptions?: ContainerOptions;
    content: Div;
    closeButton?: Parameters<typeof CloseButton>[0];
}

export default function TopLayout({
    rightTop,
    rightTopOptions,
    bottomLine,
    bottomLineOptions,
    content,
    closeButton,
}: ITopLayoutProps): Div {
    if (
        !rightTop &&
        !bottomLine &&
        !closeButton
    ) {
        return content;
    }

    return new ContainerBlock({
        orientation: 'overlap',
        width: new MatchParentSize({ weight: 1 }),
        height: new MatchParentSize({ weight: 1 }),
        items: compact([
            content,
            rightTop && new ContainerBlock({
                alignment_vertical: 'top',
                alignment_horizontal: 'right',
                ...rightTopOptions,
                items: rightTop,
            }),
            bottomLine && new ContainerBlock({
                alignment_vertical: 'bottom',
                alignment_horizontal: 'left',
                width: new MatchParentSize({ weight: 1 }),
                ...bottomLineOptions,
                items: bottomLine,
            }),
            closeButton && CloseButton({
                ...closeButton,
                options: {
                    margins: {
                        top: 36,
                        right: 36,
                    },
                    ...closeButton.options,
                },
            }),
        ]),
    });
}
