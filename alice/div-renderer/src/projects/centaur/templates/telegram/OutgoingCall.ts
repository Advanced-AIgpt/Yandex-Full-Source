import {
    TextBlock,
    SolidBackground, ContainerBlock, MatchParentSize, ImageBlock, FixedSize, WrapContentSize,
} from 'divcard2';
import { NAlice } from '../../../../protos';
import { title36m, title48m } from '../../style/Text/Text';
import { TopLevelCard } from '../../helpers/helpers';
import {
    telegramDiscardVideoCallDirective,
    bindToCallVariable,
} from '../../actions/client/telegramClientActions';
import { directivesAction } from '../../../../common/actions';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import getColorSet, { IColorSet } from '../../style/colorSet';
import { IconButton } from '../../../../common/components/IconButton';
import { colorDanger, colorWhite, offsetFromEdgeOfScreen } from '../../style/constants';
import { getStaticS3Asset } from '../../helpers/assets';
import { callControlTriggers, micControlButton, videoControlButton } from './CallControls';
import { compact } from 'lodash';
import { ExpFlags } from '../../expFlags';

export const DeviceName = 'SmartSpeaker';

interface IOutgoingCallProps {
    userId: string;
    displayName: string;
    recipientUserId: string;
    colorSet: IColorSet;
}

function OutgoingCallDiv({
    userId,
    colorSet,
    displayName,
    recipientUserId,
}: IOutgoingCallProps, requestState: IRequestState) {
    return new ContainerBlock({
        width: new MatchParentSize({ weight: 1 }),
        height: new MatchParentSize({ weight: 1 }),
        orientation: 'overlap',
        items: [
            new ImageBlock({
                image_url: 'df',
                extensions: [
                    {
                        id: 'telegram-avatar',
                        params: {
                            user_id: recipientUserId,
                        },
                    },
                ],
                width: new MatchParentSize({ weight: 1 }),
                height: new FixedSize({ value: requestState.sizes.height }),
                scale: 'fill',
            }),
            new ContainerBlock({
                margins: {
                    left: offsetFromEdgeOfScreen,
                    top: offsetFromEdgeOfScreen,
                },
                width: new WrapContentSize(),
                items: [
                    new TextBlock({
                        ...title48m,
                        text_color: colorSet.textColor,
                        text: displayName,
                        width: new WrapContentSize(),
                    }),
                    new TextBlock({
                        ...title36m,
                        text_color: colorSet.textColorOpacity50,
                        text: 'Cоединение...',
                        width: new WrapContentSize(),
                    }),
                ],
            }),
            CallButtonsContainer(userId, recipientUserId,
                requestState.hasExperiment(ExpFlags.telegramCallControlButtons)),
        ],
    });
}

function dataAdapter(data: NAlice.NData.ITOutgoingTelegramCallData): IOutgoingCallProps {
    return {
        userId: data.UserId || ' ',
        displayName: data.Recipient?.TelegramContactData?.DisplayName || ' ',
        colorSet: getColorSet(),
        recipientUserId: data.Recipient?.TelegramContactData?.UserId || ' ',
    };
}

export default function OutgoingCall(
    outgoingData: NAlice.NData.ITOutgoingTelegramCallData,
    _: MMRequest,
    requestState: IRequestState,
) {
    const options = dataAdapter(outgoingData);

    requestState.variables.add(bindToCallVariable);
    requestState.variableTriggers.add(callControlTriggers);

    return TopLevelCard({
        log_id: 'outgoing_call_card',
        states: [
            {
                state_id: 0,
                div: OutgoingCallDiv(options, requestState),
            },
        ],
    }, requestState);
}

function CallButtonsContainer(userId: string, recipientUserId: string, controlButtonsEnabled: boolean) {
    return new ContainerBlock({
        width: new WrapContentSize(),
        alignment_vertical: 'bottom',
        alignment_horizontal: 'center',
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'center',
        orientation: 'horizontal',
        margins: {
            bottom: offsetFromEdgeOfScreen + 20,
            right: 32,
        },
        items: compact([
            hangupCallButton({ recipientUserId } ),
            controlButtonsEnabled == true && micControlButton(userId),
            controlButtonsEnabled == true && videoControlButton(userId),
        ]),
    });
}

const hangupCallButton = ({
    recipientUserId,
}: {
    recipientUserId: string;
}) => {
    return IconButton({
        size: 112,
        color: colorWhite,
        iconSize: 64,
        id: 'hangup_call',
        iconUrl: getStaticS3Asset('icons/off_phone icon.png?v=3'),
        background: [
            new SolidBackground({ color: colorDanger }),
        ],
        alignment_vertical: 'bottom',
        alignment_horizontal: 'center',
        actions: [
            {
                log_id: 'video_call_hangup_call_action',
                url: directivesAction(telegramDiscardVideoCallDirective(recipientUserId)),
            },
        ],
    });
};
