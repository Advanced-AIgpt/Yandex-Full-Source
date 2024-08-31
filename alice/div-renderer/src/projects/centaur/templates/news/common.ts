import {
    ContainerBlock,
    DivBackground,
    FixedSize, IDivAction,
    ImageBackground,
    ImageBlock,
    MatchParentSize,
    SolidBackground,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { utcToZonedTime } from 'date-fns-tz';
import { differenceInCalendarDays, isSameDay } from 'date-fns';
import { Long } from 'protobufjs';
import { NAlice } from '../../../../protos';
import { formatDate } from '../../helpers/helpers';
import {
    colorDarkGrey1Opacity70,
    colorReadButton,
    colorWhite,
    colorWhiteOpacity60,
    offsetFromEdgeOfScreen,
} from '../../style/constants';
import { text40r, title32m, title60m } from '../../style/Text/Text';
import { getImageByTopic } from '../../helpers/newsHelper';
import { Avatar } from '../../../../common/helpers/avatar';

export const patchToOrig = (avatarsUrl: string) => avatarsUrl.replace(/\/[^/]+$/, '/orig');

export type NewsCardProps = {
    item: NAlice.NData.ITNewsItem;
    topic: string;
    bottomGap: boolean;
    tz: string;
};

export function newsCommonCard(
    { item: { Text, Image, Agency, Logo, PubDate, ExtendedNews }, topic, bottomGap, tz }: NewsCardProps,
    openExtendedNewsDivAction: IDivAction) {
    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        background: generateBackground(topic, Image),
        content_alignment_horizontal: 'left',
        content_alignment_vertical: 'bottom',
        paddings: {
            left: offsetFromEdgeOfScreen,
            bottom: bottomGap ? 104 : offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: [
            new ContainerBlock({
                width: new WrapContentSize(),
                height: new MatchParentSize({ weight: 1 }),
                content_alignment_vertical: 'bottom',
                orientation: 'horizontal',
                margins: {
                    bottom: 20,
                },
                items: newsSource(Agency, Logo, PubDate, tz),
            }),
            new ContainerBlock({
                width: new MatchParentSize(),
                height: new WrapContentSize(),
                content_alignment_vertical: 'bottom',
                orientation: 'horizontal',
                items: compact([
                    new TextBlock({
                        ...title60m,
                        width: new MatchParentSize({ weight: 1 }),
                        height: new WrapContentSize(),
                        max_lines: 5,
                        truncate: 'end',
                        text: Text ?? ' ',
                    }),
                    ExtendedNews?.length && new TextBlock({
                        ...text40r,
                        width: new WrapContentSize(),
                        height: new WrapContentSize(),
                        background: [new SolidBackground({
                            color: colorReadButton,
                        })],
                        border: {
                            corner_radius: 24,
                        },
                        paddings: {
                            top: 22,
                            right: 33,
                            bottom: 22,
                            left: 33,
                        },
                        margins: {
                            left: 48,
                        },
                        alignment_horizontal: 'right',
                        text: 'Читать',
                        action: openExtendedNewsDivAction,
                    }),
                ]),
            }),
        ],
    });
}

export const newsSource = (
    Agency: (string | null | undefined),
    Logo: (string | null | undefined),
    PubDate: (number | Long | null | undefined),
    tz: string,
) =>
    compact([
        Logo && new ContainerBlock({
            background: [
                new SolidBackground({
                    color: colorWhite,
                }),
            ],
            content_alignment_horizontal: 'center',
            content_alignment_vertical: 'center',
            border: {
                corner_radius: 12,
            },
            width: new FixedSize({ value: 40 }),
            height: new FixedSize({ value: 40 }),
            margins: {
                right: 20,
            },
            items: [
                new ImageBlock({
                    width: new FixedSize({ value: 28 }),
                    height: new FixedSize({ value: 28 }),
                    image_url: Logo ?? '',
                }),
            ],
        }),
        new TextBlock({
            ...title32m,
            text_color: colorWhiteOpacity60,
            width: new WrapContentSize(),
            height: new WrapContentSize(),
            text_alignment_vertical: 'center',
            text: (Agency ? Agency + ' • ' : '') + (Number(PubDate) ? formDate(Number(PubDate) * 1000 || 0, tz) : ''),
        }),
    ]);

export const generateBackground = (
    Topic: string | null | undefined,
    NewsImage: NAlice.NData.TNewsItem.ITImage | null | undefined,
    color = colorDarkGrey1Opacity70,
) => {
    const result: DivBackground[] = [];
    if (NewsImage && NewsImage.Src) {
        const avatar = Avatar.fromUrl(NewsImage.Src);

        result.push(
            new ImageBackground({
                image_url: avatar ? avatar.setTypeName('orig').toString() : NewsImage.Src,
                scale: 'fill',
                preload_required: 1,
            }),
        );
    } else {
        result.push(
            new ImageBackground({
                image_url: getImageByTopic(Topic),
                preload_required: 1,
            }),
        );
    }

    result.push(new SolidBackground({ color }));
    return result;
};

export const formDate = (date: number, tz: string) => {
    const d = utcToZonedTime(date, tz);
    const now = utcToZonedTime(Date.now(), tz);
    if (isSameDay(d, now)) {
        return formatDate(d, 'HH:mm');
    }
    if (differenceInCalendarDays(now, d) === 1) {
        return `вчера в ${formatDate(d, 'HH:mm')}`;
    }
    return formatDate(d, 'd MMMM в HH:mm');
};
