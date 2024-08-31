import { ContainerBlock, Div, FixedSize, SeparatorBlock, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { chain, compact } from 'lodash';
import { NAlice } from '../../../../../protos';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { insertBetween } from '../../../helpers/helpers';
import { ContainerBlockProps } from '../../../helpers/types';
import { colorBlueText, colorBlueTextOpacity50, offsetFromEdgeOfScreen } from '../../../style/constants';
import { title36m } from '../../../style/Text/Text';
import { SectionTemplate } from './SearchRichCardSection';

interface FactIcon {
    color: string;
    type: NAlice.NData.TSimpleText.TTextSymbol.ESymbolType;
}
interface FactMultilineText {
    text: string;
    icon?: FactIcon;
}
interface MultilineFact {
    fact: string;
    multilineAnswer: FactMultilineText[];
}
interface Fact {
    fact: string;
    answer: string;
}

interface FactListMultilineAnswerIconProps extends ContainerBlockProps {
    icon: FactIcon;
}
const FactListMultilineAnswerIcon = ({ icon, ...props }: FactListMultilineAnswerIconProps) => {
    switch(icon.type) {
        case NAlice.NData.TSimpleText.TTextSymbol.ESymbolType.Subway: {
            return new ContainerBlock({
                width: new FixedSize({ value: 46 }),
                height: new FixedSize({ value: 46 }),
                paddings: {
                    top: 6,
                    left: 6,
                    right: 6,
                    bottom: 6,
                },
                items: [
                    new SeparatorBlock({
                        height: new FixedSize({ value: 34 }),
                        width: new FixedSize({ value: 34 }),
                        background: [new SolidBackground({ color: icon.color })],
                        border: { corner_radius: 1000 },
                    }),
                ],
                ...props,
            });
        }
    }

    return undefined;
};

interface FactListMultilineAnswerProps {
    multilineAnswer: FactMultilineText[];
}
const FactListMultilineAnswer = ({ multilineAnswer }: FactListMultilineAnswerProps) => {
    const isOnlyText = multilineAnswer.every(({ icon }) => typeof icon === 'undefined');

    if (isOnlyText) {
        const text = multilineAnswer.map(({ text }) => text).join(', ');

        return new TextBlock({
            ...title36m,
            text_color: colorBlueText,
            text,
            width: new FixedSize({ value: 832 }),
            margins: { left: 48 },
        });
    }

    const items = chain(multilineAnswer)
        // формируем ноды ответа
        .map(({ text, icon }) => {
            const items: Div[] = [];

            if (icon) {
                const iconResult = FactListMultilineAnswerIcon({ icon, margins: { right: 6 } });

                if (iconResult) {
                    items.push(iconResult);
                }
            }

            items.push(new TextBlock({
                ...title36m,
                text_color: colorBlueText,
                text,
            }));

            return new ContainerBlock({
                orientation: 'horizontal',
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                items,
            });
        })
        // разделяем ноды ответа на пачки по 2
        .chunk(2)
        // добавляем разделитель между нодами ответа
        .map(row => insertBetween(
            row,
            new SeparatorBlock({ width: new FixedSize({ value: 16 }) }),
        ))
        // оборачиваем пачки по 2 ноды ответа с разделителем в контейнер строки
        .map(items => new ContainerBlock({ items, orientation: 'horizontal', width: new WrapContentSize() }))
        .value();

    return new ContainerBlock({
        orientation: 'vertical',
        margins: { left: 48 },
        width: new FixedSize({ value: 832 }),
        items,
    });
};

interface FactListRowProps {
    fact: Fact | MultilineFact;
}
const FactListRow = ({ fact }: FactListRowProps) => {
    const items: Div[] = [];

    items.push(new TextBlock({
        width: new FixedSize({ value: 304 }),
        ...title36m,
        text_color: colorBlueTextOpacity50,
        text: fact.fact,
    }));

    // как правило, есть либо answer, либо multilineAnswer
    if ('answer' in fact) {
        items.push(
            new TextBlock({
                width: new FixedSize({ value: 832 }),
                ...title36m,
                text_color: colorBlueText,
                text: fact.answer,
                margins: { left: 48 },
            }),
        );
    }

    // как правило, есть либо answer, либо multilineAnswer
    if ('multilineAnswer' in fact) {
        items.push(FactListMultilineAnswer({ multilineAnswer: fact.multilineAnswer }));
    }

    return new ContainerBlock({ items, orientation: 'horizontal' });
};

interface FactListProps {
    facts: (Fact | MultilineFact)[];
}
const FactList = ({ facts }: FactListProps) => {
    const rows = facts.map<Div>(fact => FactListRow({ fact }));

    return new ContainerBlock({
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: insertBetween(rows, new SeparatorBlock({ height: new FixedSize({ value: 48 }) })),
    });
};

type FactListSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'FactList'>;

const schema = {
    type: 'object',
    properties: {
        FactList: {
            type: 'object',
            properties: {
                Facts: {
                    type: 'array',
                    items: {
                        anyOf: [
                            {
                                type: 'object',
                                required: ['FactText', 'TextAnswer'],
                                properties: {
                                    FactText: { type: 'string' },
                                    TextAnswer: { type: 'string' },
                                },
                            },
                            {
                                type: 'object',
                                required: ['FactText', 'MultiTextAnswer'],
                                properties: {
                                    FactText: { type: 'string' },
                                    MultiTextAnswer: {
                                        type: 'object',
                                        required: ['TextAnswer'],
                                        properties: {
                                            TextAnswer: {
                                                type: 'array',
                                                minItems: 1,
                                                items: {
                                                    type: 'object',
                                                    required: ['Text', 'Symbol'],
                                                    properties: {
                                                        Text: { type: 'string' },
                                                        Symbol: {
                                                            type: 'object',
                                                            required: ['Type', 'Color'],
                                                            properties: {
                                                                Type: { type: 'number' },
                                                                Color: { type: 'string' },
                                                            },
                                                            nullable: true,
                                                        },
                                                    },
                                                },
                                            },
                                        },
                                    },
                                },
                            },
                        ],
                    },
                    minItems: 1,
                },
            },
            required: ['Facts'],
        },
    },
    required: ['FactList'],
};

const isAnswerValid = (text: string) => text.startsWith('http') === false;

const dataAdapter = createDataAdapter(
    schema,
    (section: FactListSection) => {
        const data = section.FactList?.Facts ?? [];

        const facts = compact(data.map<Fact | MultilineFact | undefined>(({ FactText, TextAnswer, MultiTextAnswer }) => {
            if (FactText && TextAnswer && isAnswerValid(TextAnswer)) {
                return {
                    fact: FactText,
                    answer: TextAnswer,
                };
            }

            if (MultiTextAnswer?.TextAnswer) {
                const multilineAnswer = compact(MultiTextAnswer.TextAnswer.map(({ Text, Symbol }) => {
                    if (!Text || isAnswerValid(Text) === false) {
                        return undefined;
                    }

                    const icon = (() => {
                        if (!Symbol?.Color || !Symbol?.Type) {
                            return undefined;
                        }

                        return {
                            color: Symbol.Color,
                            type: Symbol.Type,
                        };
                    })();

                    return {
                        text: Text,
                        icon,
                    };
                }));

                if (multilineAnswer.length === 0 || !FactText) {
                    return undefined;
                }

                return {
                    fact: FactText,
                    multilineAnswer,
                };
            }

            return undefined;
        }));

        return { facts };
    },
);

export const SearchRichCardFactListSection: SectionTemplate = (section, requestState) => {
    const { facts } = dataAdapter(section, requestState);

    if (facts.length === 0) {
        return undefined;
    }

    return FactList({ facts });
};
