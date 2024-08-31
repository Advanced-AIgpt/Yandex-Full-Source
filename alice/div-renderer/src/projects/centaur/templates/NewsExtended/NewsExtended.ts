import { ContainerBlock, FixedSize, GalleryBlock, MatchParentSize, WrapContentSize } from 'divcard2';
import { compact } from 'lodash';
import CloseButton from '../../components/CloseButton';
import { offsetFromEdgeOfScreen } from '../../style/constants';
import { setStateAction } from '../../../../common/actions/div';
import { DualScreen } from '../../components/DualScreen/DualScreen';
import { NewsCard } from '../NewsCard/NewsCard';
import { NewsCardProps } from '../news/common';
import { NewsTitle } from '../NewsTitle/NewsTitle';
import getColorSet from '../../style/colorSet';
import { IRequestState } from '../../../../common/types/common';

export function NewsExtended(props: NewsCardProps, requestState: IRequestState) {
    const { item, topic, tz } = props;
    const theme = getColorSet();

    return DualScreen({
        requestState,
        firstDiv: [NewsTitle({ item, topic, tz, theme }, true)],
        secondDiv: [new ContainerBlock({
            width: new MatchParentSize(),
            height: new MatchParentSize(),
            orientation: 'overlap',
            items: [
                new GalleryBlock({
                    width: new WrapContentSize(),
                    height: new FixedSize({ value: 853 }),
                    orientation: 'vertical',
                    paddings: {
                        top: 50,
                    },
                    items: [
                        ...compact(item.ExtendedNews?.map((news, newsIndex) => {
                            const color = newsIndex % 2 === 0 ? theme.mainColor1 : theme.mainColor;
                            return NewsCard(news, theme, color);
                        })),
                    ],
                }),
                CloseButton({
                    padding: 16,
                    options: {
                        margins: {
                            top: offsetFromEdgeOfScreen,
                            right: 24,
                        },
                        actions: [
                            {
                                log_id: 'close_fullscreen',
                                url: setStateAction('0/news/pager', true),
                            },
                        ],
                    },
                    preventDefault: true,
                    backgroundColor: theme.closeButtonBackgroundColor, //TODO: backdrop-filter: blur(88px) &opacity prop
                    borderRadius: 100,
                    size: 72,
                }),
            ],
        })],
        mainColor1: theme.mainColor1,
        mainColor: theme.mainColor,
    });
}
