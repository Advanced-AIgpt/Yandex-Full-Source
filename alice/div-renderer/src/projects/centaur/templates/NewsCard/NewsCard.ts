import { ContainerBlock, MatchParentSize, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { text28r, text34r } from '../../style/Text/Text';
import { IColorSet } from '../../style/colorSet';

export function NewsCard(news: NAlice.NData.ITNewsItem, theme: IColorSet, background: string) {
    return new ContainerBlock({
        height: new WrapContentSize(),
        width: new MatchParentSize(),
        paddings: {
            top: 40,
            left: 48,
            right: 48,
            bottom: 40,
        },
        items: compact([
            news.Text && new TextBlock({
                ...text34r,
                text: news.Text,
                text_color: theme.textColor,
                alpha: 0.9,
                paddings: {
                    bottom: 16,
                },
            }),
            news.Agency && new TextBlock({
                ...text28r,
                text: news.Agency.toUpperCase(),
                text_color: theme.textColor,
                alpha: 0.5,
            }),
        ]),
        background: [new SolidBackground({ color: background })],
    });
}
