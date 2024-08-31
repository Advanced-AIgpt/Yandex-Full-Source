import { ContainerBlock, Div, DivStateBlock, FixedSize, ImageBlock, MatchParentSize, TextBlock } from 'divcard2';
import { compact } from 'lodash';
import { BasicCard } from '../BasicCard';
import { title32m } from '../../../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../../../style/constants';
import { getS3Asset } from '../../../../helpers/assets';
import { IEmptyCardProps } from './types';
import { LoaderDiv } from '../../../../components/Loader/Loader';
import { setStateAction } from '../../../../../../common/actions/div';
import { getCardMainScreenId } from '../helpers';

const ADD_WIDGET_CARD = 'add_widget_loading';
const ADD_WIDGET_CARD_ON = 'on';
const ADD_WIDGET_CARD_OFF = 'off';

export default function EmptyCard({ actions, longtap_actions, rowIndex, colIndex } : IEmptyCardProps): Div {
    const divId = `${ADD_WIDGET_CARD}_${rowIndex ?? 0}_${colIndex ?? 0}`;

    return BasicCard({
        id: getCardMainScreenId({ colIndex, rowIndex }),
        height: new MatchParentSize(),
        width: new MatchParentSize(),
        items: [
            new DivStateBlock({
                div_id: divId,
                default_state_id: ADD_WIDGET_CARD_OFF,
                states: [
                    {
                        state_id: ADD_WIDGET_CARD_OFF,
                        div: new ContainerBlock({
                            height: new MatchParentSize(),
                            width: new MatchParentSize(),
                            actions: actions ? compact([
                                {
                                    log_id: 'mainscreen.emptycard.change.loading.state',
                                    url: setStateAction([
                                        '0',
                                        divId,
                                        ADD_WIDGET_CARD_ON,
                                    ]),
                                },
                                ...actions,
                            ]) : [],
                            longtap_actions,
                            items: [
                                new ImageBlock({
                                    height: new FixedSize({ value: 88 }),
                                    width: new FixedSize({ value: 88 }),
                                    margins: {
                                        top: 24,
                                        bottom: 24,
                                    },
                                    image_url: getS3Asset('icons/plus_in_circle.png'),
                                    alignment_horizontal: 'center',
                                }),
                                new TextBlock({
                                    ...title32m,
                                    text_color: colorWhiteOpacity50,
                                    width: new MatchParentSize(),
                                    height: new MatchParentSize(),
                                    text_alignment_horizontal: 'center',
                                    text: 'Добавить\nвиджет',
                                }),
                            ],
                        }),
                    },
                    {
                        state_id: ADD_WIDGET_CARD_ON,
                        div: new ContainerBlock({
                            height: new MatchParentSize(),
                            width: new MatchParentSize(),
                            actions: [
                                {
                                    log_id: 'mainscreen.emptycard.change.loading.state',
                                    url: setStateAction([
                                        '0',
                                        divId,
                                        ADD_WIDGET_CARD_OFF,
                                    ]),
                                },
                            ],
                            items: [
                                LoaderDiv(),
                            ],
                        }),
                    },
                ],
            }),
        ],
    });
}
