import {
    ContainerBlock,
    GalleryBlock,
    TextBlock, WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { text40m, title32m, title60m } from '../../style/Text/Text';
import { NewsCardProps, newsSource } from './common';
import { closeButtonDefaultSize } from '../../components/CloseButton';
import { colorWhiteOpacity50, offsetFromEdgeOfScreen } from '../../style/constants';

export const CLOSE_BUTTON_PADDING = 10;

const CONTENT_RIGHT_PADDING = offsetFromEdgeOfScreen * 2 + closeButtonDefaultSize + CLOSE_BUTTON_PADDING * 2;

export function renderExtendedNewsContent(
    { item: { Text, Agency, Logo, PubDate, ExtendedNews }, topic, bottomGap, tz }: NewsCardProps) {
    return new GalleryBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        margins: {
            right: CONTENT_RIGHT_PADDING,
        },
        orientation: 'vertical',
        items: compact([
            renderSource({ item: { Agency, Logo, PubDate }, topic, bottomGap, tz }),
            renderTitle({ item: { Text }, topic, bottomGap, tz }),
            ...renderBullets({ item: { ExtendedNews }, topic, bottomGap, tz }),
        ]),
    });
}

function renderSource({ item: { Agency, Logo, PubDate }, tz }: NewsCardProps) {
    return new ContainerBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        orientation: 'horizontal',
        margins: {
            top: offsetFromEdgeOfScreen,
            bottom: 24,
        },
        items: newsSource(Agency, Logo, PubDate, tz ?? ''),
    });
}

function renderTitle({ item: { Text } }: NewsCardProps) {
    return new TextBlock({
        ...title60m,
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        truncate: 'end',
        margins: {
            bottom: 32,
        },
        text: Text ?? ' ',
    });
}

function renderBullets({ item: { ExtendedNews } }: NewsCardProps) {
    return compact(ExtendedNews?.map((item, i, arr) => {
        const text = item?.Text ?? '';
        const agency = item?.Agency ?? '';
        const last = (i !== arr.length - 1);

        return new TextBlock({
            width: new WrapContentSize(),
            height: new WrapContentSize(),
            margins: {
                bottom: last ? 32 : offsetFromEdgeOfScreen,
            },
            ranges: [{
                start: 0,
                end: text.length ?? 0,
                ...text40m,
            }, {
                ...title32m,
                text_color: colorWhiteOpacity50,
                start: text.length ?? 0,
                end: (text.length ?? 0) + ('  ' + agency).length,
            }],
            text: text + '  ' + agency,
        });
    }));
}
