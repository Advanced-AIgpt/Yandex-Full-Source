import { Avatar } from '../../../../../common/helpers/avatar';
import { NAlice } from '../../../../../protos';
import { CompanyGallery, ICompanyGalleryItem } from '../../../components/CompanyGallery/CompanyGallery';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { offsetFromEdgeOfScreen } from '../../../style/constants';
import { SectionTemplate } from './SearchRichCardSection';

type CompaniesSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'Companies'>;

const schema = {
    type: 'object',
    required: ['Companies'],
    properties: {
        Companies: {
            type: 'object',
            required: ['Companies'],
            properties: {
                Companies: {
                    type: 'array',
                    minItems: 1,
                    items: {
                        type: 'object',
                        required: ['Name', 'Image', 'Rating', 'Address'],
                        properties: {
                            Name: { type: 'string' },
                            Image: {
                                type: 'object',
                                required: ['Url'],
                                properties: {
                                    Url: { type: 'string' },
                                },
                                nullable: true,
                            },
                            Rating: { type: 'string' },
                            Address: { type: 'string' },
                        },
                    },
                },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (section: CompaniesSection) => {
    const data = section.Companies?.Companies ?? [];

    const companies = data.map<ICompanyGalleryItem>(({ Image, Name, Rating, Address }) => {
        const parsedRating = Number(Rating);
        const imageUrl = (() => {
            if (!Image?.Url) {
                return ' ';
            }

            return Avatar.setImageSize({
                data: Image.Url,
                size: 'mediumRectangle',
                namespace: 'get-altay',
            });
        })();

        return {
            imageUrl,
            title: Name ?? ' ',
            subtitle: Address ?? ' ',
            rating: Number.isNaN(parsedRating) ? undefined : parsedRating,
        };
    });

    return { companies };
});

export const SearchRichCardCompaniesSection: SectionTemplate = (section, requestState) => {
    const { companies } = dataAdapter(section, requestState);

    return CompanyGallery({
        companies,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
    });
};
