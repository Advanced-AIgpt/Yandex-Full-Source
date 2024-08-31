import { ContainerBlock, FixedSize, GalleryBlock, IDivAction, ImageBlock, TextBlock } from 'divcard2';
import { Avatar } from '../../../../../common/helpers/avatar';
import { NAlice } from '../../../../../protos';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { colorBlueText, offsetFromEdgeOfScreen } from '../../../style/constants';
import { text28m } from '../../../style/Text/Text';
import { getGoToOOTextAction } from '../SearchRichCard.tools';
import { SectionTemplate } from './SearchRichCardSection';

interface Band {
    title: string;
    imageUrl: string;
    action?: IDivAction;
}
interface BandsGalleryProps {
    bands: Band[];
}
const BandsGallery = ({ bands }: BandsGalleryProps) => {
    return new GalleryBlock({
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: bands.map(({ title, imageUrl, action }) => {
            return new ContainerBlock({
                height: new FixedSize({ value: 340 }),
                width: new FixedSize({ value: 284 }),
                action,
                items: [
                    new ImageBlock({
                        width: new FixedSize({ value: 284 }),
                        height: new FixedSize({ value: 284 }),
                        image_url: imageUrl,
                        border: { corner_radius: 1000 },
                    }),
                    new TextBlock({
                        ...text28m,
                        margins: { top: 20 },
                        text_color: colorBlueText,
                        text: title,
                        text_alignment_horizontal: 'center',
                    }),
                ],
            });
        }),
    });
};

type MusicBandsSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'MusicBands'>;

const schema = {
    type: 'object',
    properties: {
        MusicBands: {
            type: 'object',
            properties: {
                Bands: {
                    type: 'array',
                    items: {
                        type: 'object',
                        properties: {
                            Logo: {
                                type: 'object',
                                properties: {
                                    Url: { type: 'string' },
                                },
                                required: ['Url'],
                            },
                            Title: { type: 'string' },
                        },
                        required: ['Logo', 'Title'],
                    },
                    minItems: 1,
                },
            },
            required: ['Bands'],
        },
    },
    required: ['MusicBands'],
};

const dataAdapter = createDataAdapter(
    schema,
    (section: MusicBandsSection) => {
        const data = section.MusicBands?.Bands ?? [];

        const bands = data.map<Band>(({ Title, Logo }) => {
            const title = Title ?? ' ';
            const imageUrl = (() => {
                if (!Logo?.Url) {
                    return ' ';
                }

                return Avatar.setImageSize({
                    data: Logo?.Url,
                    size: 'largeSquare',
                    namespace: 'get-entity_search',
                });
            })();

            return {
                title,
                imageUrl,
                action: getGoToOOTextAction(title),
            };
        });

        return { bands };
    },
);


export const SearchRichCardMusicBandsSection: SectionTemplate = (section, requestState) => {
    const { bands } = dataAdapter(section, requestState);

    if (bands.length === 0) {
        return undefined;
    }

    return BandsGallery({ bands });
};
