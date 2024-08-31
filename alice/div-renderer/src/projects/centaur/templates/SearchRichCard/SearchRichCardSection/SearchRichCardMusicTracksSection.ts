import { ContainerBlock, Div, FixedSize, GalleryBlock, ImageBlock, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../../protos';
import { formatTime } from '../../../helpers/time';
import { colorBlueText, colorBlueTextOpacity50, offsetFromEdgeOfScreen } from '../../../style/constants';
import { text28m } from '../../../style/Text/Text';
import { SectionTemplate } from './SearchRichCardSection';
import { ContainerBlockProps, GalleryBlockProps } from '../../../helpers/types';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { Avatar } from '../../../../../common/helpers/avatar';

interface Track {
    title: string;
    subtitle: string;
    imageUrl: string;
}

interface TracksGalleryItemProps extends ContainerBlockProps {
    track: Track;
}
const TracksGalleryItem = ({ track, ...props }: TracksGalleryItemProps) => {
    return new ContainerBlock({
        ...props,
        orientation: 'horizontal',
        background: [new SolidBackground({ color: '#374352' })],
        border: { corner_radius: 24 },
        height: new FixedSize({ value: 128 }),
        width: new FixedSize({ value: 584 }),
        paddings: {
            top: 24,
            left: 24,
            right: 24,
            bottom: 24,
        },
        items: [
            new ImageBlock({
                border: { corner_radius: 16 },
                image_url: track.imageUrl,
                height: new FixedSize({ value: 80 }),
                width: new FixedSize({ value: 80 }),
            }),
            new ContainerBlock({
                margins: {
                    left: 24,
                },
                items: [
                    new TextBlock({
                        ...text28m,
                        text: track.title,
                        text_color: colorBlueText,
                    }),
                    new TextBlock({
                        ...text28m,
                        text_color: colorBlueTextOpacity50,
                        text: track.subtitle,
                    }),
                ],
            }),
        ],
    });
};

interface MusicTracksGalleryProps extends GalleryBlockProps {
    tracks: Track[];
}
const MusicTracksGallery = ({ tracks, ...props }: MusicTracksGalleryProps) => {
    const items: Div[] = [];

    for (let index = 0; index < tracks.length; index = index + 2) {
        const current = tracks[index];
        const next = tracks[index + 1];

        items.push(
            new ContainerBlock({
                width: new WrapContentSize(),
                items: compact([
                    current && TracksGalleryItem({ track: current }),
                    next && TracksGalleryItem({ track: next, margins: { top: 16 } }),
                ]),
            }),
        );
    }

    return new GalleryBlock({
        ...props,
        item_spacing: 16,
        items,
    });
};

type MusicTracksSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'MusicTracks'>;

const schema = {
    type: 'object',
    properties: {
        MusicTracks: {
            type: 'object',
            properties: {
                Tracks: {
                    type: 'array',
                    items: {
                        type: 'object',
                        properties: {
                            Title: { type: 'string' },
                            ArtImageUrl: { type: 'string' },
                            Subtype: { type: 'string' },
                            DurationMs: { type: 'number' },
                        },
                        required: ['Title', 'ArtImageUrl', 'Subtype', 'DurationMs'],
                    },
                    minItems: 1,
                },
            },
            required: ['Tracks'],
        },
    },
    required: ['MusicTracks'],
};

const dataAdapter = createDataAdapter(schema, (section: MusicTracksSection) => {
    const data = section.MusicTracks?.Tracks ?? [];

    const tracks = data.map<Track>(({ Title, Album, ArtImageUrl, DurationMs, Subtype }) => {
        const imageUrl = (() => {
            if (!ArtImageUrl) {
                return ' ';
            }

            return Avatar.setImageSize({
                data: ArtImageUrl,
                size: 'smallSquare',
                namespace: 'get-entity_search',
            });
        })();
        const subtitle = (() => {
            // TODO: CENTAUR-1236: проверить данные альбомов у треков
            if (Album?.Title && Album?.ReleaseYear) {
                const duration = DurationMs ?? 0;

                return compact([
                    [Album.Title, Album.ReleaseYear].join(' '),
                    duration > 0 ? formatTime(duration) : undefined,
                ]).join(' · ');
            }

            return Subtype ?? ' ';
        })();

        return {
            title: Title ?? ' ',
            subtitle,
            imageUrl,
        };
    });

    return { tracks };
});

export const SearchRichCardMusicTracksSection: SectionTemplate = (section, requestState) => {
    const { tracks } = dataAdapter(section, requestState);

    return MusicTracksGallery({
        tracks,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
    });
};
