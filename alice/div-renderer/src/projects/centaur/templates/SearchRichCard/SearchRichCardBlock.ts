import { ContainerBlock, TextBlock } from 'divcard2';
import { NAlice } from '../../../../protos';
import { insertBetween } from '../../helpers/helpers';
import { colorBlueText, offsetFromEdgeOfScreen } from '../../style/constants';
import { title48m } from '../../style/Text/Text';
import { SectionSeparator } from './SearchRichCard.tools';
import { SearchRichCardSection } from './SearchRichCardSection/SearchRichCardSection';
import { TextBlockProps } from '../../helpers/types';
import { IRequestState } from '../../../../common/types/common';
import { compact } from 'lodash';
import { createDataAdapter } from '../../helpers/createDataAdapter';

interface BlockHeaderProps extends TextBlockProps {
    text: string;
}
const BlockHeader = ({ text, ...props }: BlockHeaderProps) => {
    return new TextBlock({
        ...props,
        ...title48m,
        text_color: colorBlueText,
        text,
    });
};

const schema = {
    type: 'object',
    required: ['Title', 'Sections'],
    properties: {
        Title: { type: 'string' },
        Sections: {
            type: 'array',
            minItems: 1,
        },
    },
};

const dataAdapter = createDataAdapter(schema, (block: NAlice.NData.TSearchRichCardData.ITBlock) => {
    const sectionDataList = block.Sections ?? [];
    const title = block.Title ?? ' ';

    return { sectionDataList, title };
});

export const SearchRichCardBlock = (block: NAlice.NData.TSearchRichCardData.ITBlock, requestState: IRequestState) => {
    const { sectionDataList, title } = dataAdapter(block, requestState);

    const sections = compact(sectionDataList.map(section => SearchRichCardSection(section, requestState)));

    if (sections.length === 0) {
        return undefined;
    }

    return new ContainerBlock({
        items: [
            BlockHeader({
                text: title,
                margins: {
                    bottom: 32,
                    left: offsetFromEdgeOfScreen,
                    right: offsetFromEdgeOfScreen,
                },
                paddings: { top: 48 },
            }),
            ...insertBetween(sections, SectionSeparator()),
        ],
    });
};
