import { ContainerBlock } from 'divcard2';
import { compact } from 'lodash';
import { IRequestState } from '../../../../common/types/common';
import { NAlice } from '../../../../protos';
import { createDataAdapter } from '../../helpers/createDataAdapter';
import { insertBetween } from '../../helpers/helpers';
import { isGalleryBlock, isMainBlock, SectionSeparator } from './SearchRichCard.tools';
import { SearchRichCardSection } from './SearchRichCardSection/SearchRichCardSection';

const schema = {
    type: 'object',
    required: ['Blocks'],
    properties: {
        Blocks: {
            type: 'array',
            minItems: 1,
            items: {
                anyOf: [
                    // main block
                    {
                        type: 'object',
                        required: ['BlockType', 'Sections'],
                        properties: {
                            BlockType: { type: 'number', minimum: 1, maximum: 1 },
                            Sections: {
                                type: 'array',
                                minItems: 1,
                            },
                        },
                    },
                    // other blocks
                    {
                        type: 'object',
                        required: ['BlockType', 'Sections'],
                        properties: {
                            BlockType: { type: 'number' },
                            Sections: { type: 'array', minItems: 1 },
                        },
                    },
                ],
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (data: NAlice.NData.ITSearchRichCardData) => {
    const mainBlock = (data.Blocks ?? []).find(block => isMainBlock(block));
    const galleryBlock = (data.Blocks ?? []).find(block => isGalleryBlock(block));

    const sectionsToRender = [...(galleryBlock?.Sections ?? []), ...(mainBlock?.Sections ?? [])];

    return { sectionsToRender };
});

export const SearchRichCardMainBlock = (data: NAlice.NData.ITSearchRichCardData, requestState: IRequestState) => {
    const { sectionsToRender } = dataAdapter(data, requestState);

    if (sectionsToRender.length === 0) {
        return undefined;
    }

    return new ContainerBlock({
        paddings: { top: 48 },
        items: [
            ...insertBetween(
                compact(sectionsToRender.map(section => SearchRichCardSection(section, requestState))),
                SectionSeparator(),
            ),
        ],
    });
};
