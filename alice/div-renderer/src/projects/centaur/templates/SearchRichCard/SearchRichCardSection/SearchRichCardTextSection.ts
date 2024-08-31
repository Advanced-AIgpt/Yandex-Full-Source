import { ContainerBlock, TextBlock } from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../../protos';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { colorBlueText, colorBlueTextOpacity50, offsetFromEdgeOfScreen } from '../../../style/constants';
import { text40m } from '../../../style/Text/Text';
import { SectionTemplate } from './SearchRichCardSection';

type TextSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'Text'>;

const schema = {
    type: 'object',
    required: ['Text'],
    properties: {
        Text: {
            type: 'object',
            required: ['Text', 'Hostname'],
            properties: {
                Text: { type: 'string' },
                Hostname: { type: 'string' },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (section: TextSection) => {
    const data = section.Text;

    return {
        text: data?.Text ?? ' ',
        source: data?.Hostname ?? ' ',
    };
});

export const SearchRichCardTextSection: SectionTemplate = (section, requestState) => {
    const { text, source } = dataAdapter(section, requestState);

    return new ContainerBlock({
        orientation: 'vertical',
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: [
            new TextBlock({
                ...text40m,
                text_color: colorBlueText,
                ranges: [
                    {
                        start: text.length,
                        // +1 потому что пробел между текстом и источником
                        end: text.length + 1 + source.length,
                        text_color: colorBlueTextOpacity50,
                    },
                ],
                text: compact([text, source]).join(' '),
            }),
        ],
    });
};
