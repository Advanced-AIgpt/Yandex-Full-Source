import { ContainerBlock, FixedSize, GalleryBlock, ImageBlock, MatchParentSize, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { NAlice } from '../../../../../protos';
import { formatTime } from '../../../helpers/time';
import { colorBlueText, colorBlueTextOpacity50, colorWhite, offsetFromEdgeOfScreen } from '../../../style/constants';
import { text28m } from '../../../style/Text/Text';
import { SectionTemplate } from './SearchRichCardSection';
import { GalleryBlockProps } from '../../../helpers/types';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { compact } from 'lodash';
import { Avatar } from '../../../../../common/helpers/avatar';

interface Clip {
    imageUrl: string;
    title: string;
    subtitle: string;
    durationSeconds: number;
}
interface ClipGalleryProps extends GalleryBlockProps {
    clips: Clip[];
}
const ClipGallery = ({ clips, ...props }: ClipGalleryProps) => {
    return new GalleryBlock({
        ...props,
        item_spacing: 16,
        items: clips.map(({ durationSeconds, imageUrl, title, subtitle }) => {
            const time = durationSeconds > 0 ? formatTime(durationSeconds) : undefined;

            return new ContainerBlock({
                orientation: 'overlap',
                height: new FixedSize({ value: 310 }),
                width: new FixedSize({ value: 384 }),
                items: [
                    new ContainerBlock({
                        orientation: 'overlap',
                        height: new FixedSize({ value: 216 }),
                        width: new MatchParentSize(),
                        items: compact([
                            new ImageBlock({
                                image_url: imageUrl,
                                width: new MatchParentSize(),
                                height: new MatchParentSize(),
                                border: {
                                    corner_radius: 24,
                                },
                                alignment_vertical: 'top',
                            }),
                            time ? new TextBlock({
                                paddings: {
                                    top: 8,
                                    left: 12,
                                    right: 12,
                                    bottom: 8,
                                },
                                margins: {
                                    bottom: 24,
                                    right: 24,
                                },
                                border: {
                                    corner_radius: 12,
                                },
                                font_size: 22,
                                width: new WrapContentSize(),
                                text_color: colorWhite,
                                background: [
                                    new SolidBackground({ color: '#374352' }),
                                ],
                                text: time,
                                alignment_horizontal: 'right',
                                alignment_vertical: 'bottom',
                            }) : undefined,
                        ]),
                    }),
                    new ContainerBlock({
                        alignment_vertical: 'bottom',
                        items: [
                            new TextBlock({
                                ...text28m,
                                text: title,
                                text_color: colorBlueText,
                                margins: { top: 18 },
                            }),
                            new TextBlock({
                                ...text28m,
                                text: subtitle,
                                text_color: colorBlueTextOpacity50,
                                margins: { top: 4 },
                                max_lines: 1,
                            }),
                        ],
                    }),
                ],
            });
        }),
    });
};

type VideoClipsSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'VideoClips'>

const schema = {
    type: 'object',
    required: ['VideoClips'],
    properties: {
        VideoClips: {
            type: 'object',
            required: ['Videos'],
            properties: {
                Videos: {
                    type: 'array',
                    minItems: 1,
                    items: {
                        type: 'object',
                        required: ['Logo', 'Title', 'Subtitle', 'Duration'],
                        properties: {
                            Logo: {
                                type: 'object',
                                required: ['Url'],
                                properties: {
                                    Url: { type: 'string' },
                                },
                            },
                            Title: { type: 'string' },
                            Subtitle: { type: 'string' },
                            Duration: { type: 'number' },
                        },
                    },
                },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (section: VideoClipsSection) => {
    const data = section.VideoClips?.Videos ?? [];

    const clips = data.map<Clip>(({ Title, Logo, Subtitle, Duration }) => {
        const imageUrl = (() => {
            if (!Logo?.Url) {
                return ' ';
            }

            return Avatar.setImageSize({
                data: Logo.Url,
                size: 'largeRectangle',
                namespace: 'get-entity_search',
            });
        })();

        return {
            title: Title ?? ' ',
            subtitle: Subtitle ?? ' ',
            imageUrl,
            durationSeconds: Duration ?? 0,
        };
    });

    return { clips };
});

export const SearchRichCardVideoClipsSection: SectionTemplate = (section, requestState) => {
    const { clips } = dataAdapter(section, requestState);

    if (clips.length === 0) {
        return undefined;
    }

    return ClipGallery({
        clips,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
    });
};
