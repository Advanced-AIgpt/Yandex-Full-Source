import { NAlice } from '../../../../../protos';
import { offsetFromEdgeOfScreen } from '../../../style/constants';
import { SectionTemplate } from './SearchRichCardSection';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { SomeGallery, SomeGalleryItem } from '../../../components/SomeGallery/SomeGallery';
import { Avatar } from '../../../../../common/helpers/avatar';

type MusicAlbumsSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'MusicAlbums'>;

const schema = {
    type: 'object',
    properties: {
        MusicAlbums: {
            type: 'object',
            properties: {
                Albums: {
                    type: 'array',
                    items: {
                        type: 'object',
                        properties: {
                            CoverUri: { type: 'string' },
                            Title: { type: 'string' },
                            ReleaseYear: { type: 'number' },
                        },
                        required: ['CoverUri'],
                    },
                    minItems: 1,
                },
            },
            required: ['Albums'],
        },
    },
    required: ['MusicAlbums'],
};

const dataAdapter = createDataAdapter(
    schema,
    (section: MusicAlbumsSection) => {
        const data = section.MusicAlbums?.Albums ?? [];

        const some = data.map<SomeGalleryItem>(({ Title, CoverUri, ReleaseYear }) => {
            const imageUrl = (() => {
                if (!CoverUri) {
                    return ' ';
                }

                return Avatar.setImageSize({
                    data: CoverUri,
                    size: 'largeSquare',
                    namespace: 'get-entity_search',
                });
            })();

            return ({
                title: Title ?? ' ',
                imageUrl,
                subtitle: String(ReleaseYear ?? ' '),
            });
        });

        return { some };
    },
);


export const SearchRichCardMusicAlbumsSection: SectionTemplate = (section, requestState) => {
    const { some } = dataAdapter(section, requestState);

    return SomeGallery({
        some,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
    });
};
