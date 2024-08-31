import { ContainerBlock, Div, MatchParentSize, TextBlock } from 'divcard2';
import { BasicCard, IAbstractCardData } from './BasicCard';
import { title32m } from '../../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../../style/constants';
import EmptyDiv from '../../../components/EmptyDiv';

export function BasicTextCard(props: IAbstractCardData & { title: string }): Div {
    if (!props.items) {
        props.items = [];
    }

    if (Array.isArray(props.items)) {
        props.items = [
            new ContainerBlock({
                orientation: 'horizontal',
                items: [
                    new TextBlock({
                        ...title32m,
                        text_color: colorWhiteOpacity50,
                        text: props.title,
                    }),
                ],
            }),
            new EmptyDiv({
                height: new MatchParentSize(),
            }),
            ...props.items,
        ];
    }

    return BasicCard(props);
}
