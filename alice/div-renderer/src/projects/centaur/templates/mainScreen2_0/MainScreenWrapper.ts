import { ContainerBlock, Div, MatchParentSize } from 'divcard2';
import { offsetFromEdgeOfScreen, simpleBackground } from '../../style/constants';
import { MAIN_SCREEN_CARD_GAP } from './constants';

interface Props {
    cards: Div[][];
}

const gapCards = MAIN_SCREEN_CARD_GAP;

export default function MainScreenWrapper({ cards }: Props): Div {
    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        background: simpleBackground,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        orientation: 'horizontal',
        items: cards.map((el, i) => new ContainerBlock({
            height: new MatchParentSize(),
            width: new MatchParentSize(),
            margins: {
                left: i === 0 ? 0 : gapCards,
            },
            items: el.map((item, itemIndex) => new ContainerBlock({
                height: new MatchParentSize(),
                width: new MatchParentSize(),
                margins: {
                    top: itemIndex === 0 ? 0 : gapCards,
                },
                items: [item],
            })),
        })),
    });
}
