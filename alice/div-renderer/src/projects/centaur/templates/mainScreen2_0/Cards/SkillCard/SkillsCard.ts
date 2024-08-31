import {
    ContainerBlock,
    Div,
    FixedSize,
    ImageBackground,
    ImageBlock,
    MatchParentSize,
    SolidBackground,
    TextBlock,
} from 'divcard2';
import { text28m, title56m } from '../../../../style/Text/Text';
import { backgroundToImageWithText, colorWhiteOpacity10, colorWhiteOpacity50 } from '../../../../style/constants';
import { ISkillCardProps } from './types';
import { logger } from '../../../../../../common/logger';
import BasicErrorCard from '../BasicErrorCard';
import { Avatar } from '../../../../../../common/helpers/avatar';
import { BasicCard, basicCardBackground } from '../BasicCard';
import { directivesAction } from '../../../../../../common/actions';
import { createSemanticFrameAction } from '../../../../../../common/actions/server';
import { getCardMainScreenId } from '../helpers';

export const DeviceName = 'SmartSpeaker';

function getSkillTitle({ skillName, skillImage }: {
    skillName: string,
    skillImage: string,
}) {
    return new ContainerBlock({
        orientation: 'horizontal',
        items: [
            new ImageBlock({
                image_url: skillImage,
                width: new FixedSize({ value: 44 }),
                height: new FixedSize({ value: 44 }),
                border: {
                    corner_radius: 22,
                },
                margins: {
                    right: 16,
                },
            }),
            new TextBlock({
                ...text28m,
                max_lines: 1,
                text_color: colorWhiteOpacity50,
                text: skillName,
            }),
        ],
    });
}

function SkillBigButton({ text, payload }: {
    text: string;
    payload?: string;
}, skillId: string, options?: Omit<ConstructorParameters<typeof TextBlock>[0], 'text'>) {
    return new ContainerBlock({
        items: [
            new TextBlock({
                ...text28m,
                text,
                background: [
                    new SolidBackground({
                        color: colorWhiteOpacity10,
                    }),
                ],
                width: new MatchParentSize({ weight: 1 }),
                height: new MatchParentSize({ weight: 1 }),
                text_alignment_horizontal: 'center',
                text_alignment_vertical: 'center',
                border: {
                    corner_radius: 24,
                },
                ...options,
            }),
        ],
        actions: [
            {
                log_id: 'skill_action',
                url: directivesAction(
                    createSemanticFrameAction(
                        {
                            external_skill_fixed_activate_semantic_frame: {
                                fixed_skill_id: {
                                    string_value: skillId,
                                },
                                activation_command: {
                                    string_value: text,
                                },
                                payload: {
                                    string_value: payload ? payload : '',
                                },
                                activation_source_type: {
                                    string_value: 'widget_gallery',
                                },
                            },
                        },
                        'Dialogovo',
                        'alice.external_skill_widget.button',
                    ),
                ),
            },
        ],
    });
}

export default function SkillsCard({
    image,
    actions,
    title,
    comment,
    longtap_actions,
    skillName,
    skillImage,
    skillId,
    buttons,
    colIndex,
    rowIndex,
}: ISkillCardProps): Div {
    if (!skillName) {
        logger.error(new Error('The news card should have a skillName, but some of this was not transmitted.'));

        return BasicErrorCard({
            colIndex,
            rowIndex,
            title: 'Ошибка',
            description: 'Ошибка сети. Не могу показать навык. Проверьте интернет.',
            actions,
            longtap_actions,
        });
    }

    let items: Div[] = [];

    if (buttons && buttons?.length > 1) {
        items = [
            new ContainerBlock({
                orientation: 'horizontal',
                height: new MatchParentSize({ weight: 1 }),
                margins: {
                    top: 45,
                },
                items: [
                    SkillBigButton(buttons[0], skillId, {
                        margins: {
                            right: 12,
                        },
                    }),
                    SkillBigButton(buttons[1], skillId, {
                        margins: {
                            left: 12,
                        },
                    }),
                ],
            }),
        ];
    } else if (buttons && buttons.length === 1) {
        items = [
            new TextBlock({
                ...text28m,
                text: title || comment || ' ',
                max_lines: 2,
                height: new MatchParentSize({ weight: 1 }),
                text_alignment_vertical: 'center',
            }),
            SkillBigButton(buttons[0], skillId, {
                height: new FixedSize({ value: 72 }),
            }),
        ];
    } else if (title && comment) {
        items = [
            new TextBlock({
                ...(title.length < 6 ? title56m : text28m),
                text: title,
                max_lines: 2,
                height: new MatchParentSize({ weight: 1 }),
                text_alignment_vertical: 'bottom',
            }),
            new TextBlock({
                ...text28m,
                text: comment,
                alpha: 0.5,
                max_lines: 2,
            }),
        ];
    } else if (title || comment) {
        const text = (title || comment) as string;

        items = [
            new TextBlock({
                ...(text.length < 6 ? title56m : text28m),
                text,
                max_lines: 3,
                height: new MatchParentSize({ weight: 1 }),
                text_alignment_vertical: 'bottom',
            }),
        ];
    } else {
        const text = 'Что-то пошло не так.\nНавык пока не работает:(';

        items = [
            new TextBlock({
                ...(text.length < 6 ? title56m : text28m),
                text,
                max_lines: 3,
                height: new MatchParentSize({ weight: 1 }),
                text_alignment_vertical: 'bottom',
                text_color: colorWhiteOpacity50,
            }),
        ];
    }

    const cardData: Parameters<typeof BasicCard>[0] = {
        id: getCardMainScreenId({ rowIndex, colIndex }),
        items: [
            getSkillTitle({ skillName, skillImage }),
            ...items,
        ],
        actions,
        longtap_actions,
    };

    if (image) {
        cardData.background = [
            ...basicCardBackground,
            new ImageBackground({
                image_url: Avatar.fromUrl(image)?.setTypeName('380x214', 'ynews').toString() ?? image,
                scale: 'fill',
                preload_required: 1,
                content_alignment_horizontal: 'center',
                content_alignment_vertical: 'center',
            }),
            ...backgroundToImageWithText,
        ];
    }

    return BasicCard(cardData);
}
