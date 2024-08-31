import { Avatar } from '../../../../../common/helpers/avatar';
import { NAlice } from '../../../../../protos';
import { SomeGallery, SomeGalleryItem } from '../../../components/SomeGallery/SomeGallery';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { offsetFromEdgeOfScreen } from '../../../style/constants';
import { SectionTemplate } from './SearchRichCardSection';

type GeoPlacesSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'GeoPlaces'>

const schema = {
    type: 'object',
    required: ['GeoPlaces'],
    properties: {
        GeoPlaces: {
            type: 'object',
            required: ['Places'],
            properties: {
                Places: {
                    type: 'array',
                    minItems: 1,
                    items: {
                        type: 'object',
                        required: ['Name', 'Description', 'Image', 'Rating'],
                        properties: {
                            Name: { type: 'string' },
                            Description: { type: 'string' },
                            Image: {
                                type: 'object',
                                required: ['Url'],
                                properties: {
                                    Url: { type: 'string' },
                                },
                            },
                            Rating: { type: 'string' },
                        },
                    },
                },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (section: GeoPlacesSection) => {
    const data = section.GeoPlaces?.Places ?? [];

    const some = data.map<SomeGalleryItem>(({ Name, Image, Description }) => {
        const imageUrl = (() => {
            if (!Image?.Url) {
                return ' ';
            }

            return Avatar.setImageSize({
                data: Image.Url,
                size: 'largeSquare',
                namespace: 'get-entity_search',
            });
        })();

        return {
            title: Name || ' ',
            imageUrl,
            subtitle: Description || ' ',
        };
    });

    return { some };
});

export const SearchRichCardGeoPlacesSection: SectionTemplate = (section, requestState) => {
    const { some } = dataAdapter(section, requestState);

    return SomeGallery({ some, paddings: { left: offsetFromEdgeOfScreen, right: offsetFromEdgeOfScreen } });
};
