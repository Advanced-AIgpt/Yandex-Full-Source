import {
    ContainerBlock,
    FixedSize,
    MatchParentSize,
    TextBlock,
    WrapContentSize,
    SolidBackground,
    Div,
} from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import {
    colorWhiteOpacity50,
    colorWhiteOpacity90,
    offsetFromEdgeOfScreen,
    colorWhiteOpacity10,
    offsetTopWithSearchItem,
    simpleBackground,
} from '../../style/constants';
import { title32m, text40m } from '../../style/Text/Text';
import EmptyDiv from '../../components/EmptyDiv';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';
import { TopLevelCard } from '../../helpers/helpers';
import { directivesAction } from '../../../../common/actions';
import { telegramVideoCallTo } from '../../actions/server/telegramServerActions';
import { IRequestState } from '../../../../common/types/common';
import { MMRequest } from '../../../../common/helpers/MMRequest';

export const DeviceName = 'SmartSpeaker';

const header = new TextBlock({
    ...text40m,
    text_color: colorWhiteOpacity50,
    text: 'Уточните, кому будете звонить',
    height: new FixedSize({ value: 100 }),
    width: new WrapContentSize(),
    paddings: {
        top: 10,
        bottom: 10,
    },
});

export default function ChooseContact(
    chooseContactData: NAlice.NData.ITVideoCallContactChoosingData,
    _: MMRequest,
    requestState: IRequestState,
) {
    return TopLevelCard({
        log_id: 'choose_contact_card',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: new ContainerBlock({
                        background: simpleBackground,
                        height: new MatchParentSize(),
                        width: new MatchParentSize(),
                        paddings: {
                            top: offsetTopWithSearchItem,
                            bottom: offsetFromEdgeOfScreen,
                        },
                        orientation: 'vertical',
                        content_alignment_vertical: 'top',
                        content_alignment_horizontal: 'left',
                        ...simpleBlock(chooseContactData.ContactData),
                    } as ConstructorParameters<typeof ContainerBlock>[0]),
                }),
            },
        ],
    }, requestState);
}

const simpleBlock = (
    contactsData: NAlice.NData.ITProviderContactData[] | null | undefined,
) => {
    return {
        items: compact([
            new EmptyDiv({
                width: new MatchParentSize(),
                height: new FixedSize({ value: 30 }),
            }),
            new ContainerBlock({
                paddings: {
                    left: offsetFromEdgeOfScreen,
                    right: offsetFromEdgeOfScreen,
                },
                margins: {
                    bottom: 20,
                },
                items: compact([
                    header,
                    contactItems(contactsData),
                ]),
            }),
        ]),
    };
};

const contactItems = (
    contactsData: NAlice.NData.ITProviderContactData[] | null | undefined,
) => {
    const contactItems: Div[] = [];
    if (contactsData !== undefined && contactsData !== null) {
        for (let i = 0; i < contactsData.length; i++) {
            const contactDiv = contactItem(contactsData[i].TelegramContactData);
            if (contactDiv !== undefined && contactDiv !== null) {
                contactItems.push(contactDiv);
                if (i + 1 < contactsData.length) {
                    contactItems.push(new EmptyDiv({
                        width: new FixedSize({ value: 100 }),
                    }));
                }
            }
        }
    }

    return new ContainerBlock({
        orientation: 'horizontal',
        content_alignment_vertical: 'top',
        content_alignment_horizontal: 'left',
        width: new MatchParentSize(),
        items: contactItems,
    });
};

const contactItem = (
    telegramContactData: NAlice.NData.TProviderContactData.ITTelegramContactData | null | undefined,
) => {
    return telegramContactData && new ContainerBlock({
        orientation: 'vertical',
        width: new FixedSize({ value: 300 }),
        height: new FixedSize({ value: 500 }),
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'center',
        border: {
            corner_radius: 30,
        },
        background: [
            new SolidBackground({ color: colorWhiteOpacity10 }),
        ],
        items: [
            new TextBlock({
                ...title32m,
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                text_alignment_horizontal: 'center',
                text_alignment_vertical: 'center',
                margins: {
                    bottom: 50,
                },
                text_color: colorWhiteOpacity90,
                text: `${telegramContactData.DisplayName}`,
            }),
            telegramContactData && audioCallButton(telegramContactData),
            telegramContactData && videoCallButton(telegramContactData),
        ],
    });
};

const audioCallButton = (
    telegramContactData: NAlice.NData.TProviderContactData.ITTelegramContactData,
) => {
    return new ContainerBlock({
        orientation: 'vertical',
        width: new WrapContentSize(),
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'center',
        items: [
            new TextBlock({
                ...title32m,
                width: new FixedSize({ value: 250 }),
                height: new FixedSize({ value: 100 }),
                text_alignment_horizontal: 'center',
                text_alignment_vertical: 'center',
                text_color: colorWhiteOpacity90,
                border: {
                    corner_radius: 30,
                },
                margins: {
                    bottom: 20,
                },
                background: [
                    new SolidBackground({ color: colorWhiteOpacity10 }),
                ],
                text: 'Аудиозвонок',
            }),
        ],
        actions: compact([
            telegramContactData.UserId && {
                log_id: 'main_screen_video_call_to_action_without_video',
                url: directivesAction(telegramVideoCallTo(
                    telegramContactData.UserId,
                    telegramContactData.DisplayName,
                    false,
                )),
            },
        ]),
    });
};

const videoCallButton = (
    telegramContactData: NAlice.NData.TProviderContactData.ITTelegramContactData,
) => {
    return new ContainerBlock({
        orientation: 'vertical',
        width: new WrapContentSize(),
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'center',
        items: [
            new TextBlock({
                ...title32m,
                width: new FixedSize({ value: 250 }),
                height: new FixedSize({ value: 100 }),
                text_alignment_horizontal: 'center',
                text_alignment_vertical: 'center',
                text_color: colorWhiteOpacity90,
                border: {
                    corner_radius: 30,
                },
                margins: {
                    bottom: 20,
                },
                background: [
                    new SolidBackground({ color: colorWhiteOpacity10 }),
                ],
                text: 'Видеозвонок',
            }),
        ],
        actions: compact([
            telegramContactData.UserId && {
                log_id: 'main_screen_video_call_to_action_with_video',
                url: directivesAction(telegramVideoCallTo(
                    telegramContactData.UserId,
                    telegramContactData.DisplayName,
                    true,
                )),
            },
        ]),
    });
};
