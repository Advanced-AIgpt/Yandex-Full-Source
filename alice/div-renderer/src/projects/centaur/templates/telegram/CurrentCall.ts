import {
    SolidBackground,
    ContainerBlock,
    MatchParentSize,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { TopLevelCard } from '../../helpers/helpers';
import {
    telegramDiscardVideoCallDirective,
    bindToCallVariable,
} from '../../actions/client/telegramClientActions';
import { directivesAction } from '../../../../common/actions';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { IconButton } from '../../../../common/components/IconButton';
import { offsetFromEdgeOfScreen } from '../../style/constants';
import { getStaticS3Asset } from '../../helpers/assets';
import { TelegramCall } from '../../divCustoms/TelegramCustoms';
import { ExpFlags } from '../../expFlags';
import { callControlTriggers, micControlButton, videoControlButton } from './CallControls';

export const DeviceName = 'SmartSpeaker';

interface IProviderContactData {
    userId: string,
    displayName: string,
}

export interface ICurrentCallData {
    userId: string,
    callId: string,
    recipient: IProviderContactData,
}

export default function CurrentCall(
    currentData: NAlice.NData.ITCurrentTelegramCallData,
    _: MMRequest,
    requestState: IRequestState,
) {
    return CurrentCallCard(
        {
            userId: currentData.UserId!!,
            callId: currentData.CallId!!,
            recipient: {
                userId: currentData.Recipient!!.TelegramContactData!!.UserId!!,
                displayName: currentData.Recipient!!.TelegramContactData!!.DisplayName || '',
            },
        },
        requestState);
}

export function CurrentCallCard(
    currentData: ICurrentCallData,
    requestState: IRequestState,
) {
    requestState.variables.add(bindToCallVariable);
    requestState.variableTriggers.add(callControlTriggers);

    return TopLevelCard({
        log_id: 'outgoing_call_card',
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    width: new MatchParentSize(),
                    height: new MatchParentSize(),
                    orientation: 'overlap',
                    background: compact([
                        new SolidBackground({
                            color: '#2E000000',
                        }),
                    ]),
                    items: currentData.callId ? [
                        TelegramCall(currentData.callId),
                        CallButtonsContainer(currentData,
                            requestState.hasExperiment(ExpFlags.telegramCallControlButtons)),
                    ] : [],
                }),
            },
        ],
    }, requestState);
}

function CallButtonsContainer(currentData: ICurrentCallData, controlButtonsEnabled: boolean) {
    return new ContainerBlock({
        width: new WrapContentSize(),
        alignment_vertical: 'bottom',
        alignment_horizontal: 'center',
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'center',
        orientation: 'horizontal',
        margins: {
            bottom: offsetFromEdgeOfScreen + 20,
        },
        items: compact([
            hangupCallButton(currentData),
            controlButtonsEnabled == true && micControlButton(currentData.userId, currentData.callId),
            controlButtonsEnabled == true && videoControlButton(currentData.userId, currentData.callId),
        ]),
    });
}

const hangupCallButton = (
    currentData: ICurrentCallData,
) => {
    return IconButton({
        size: 112,
        color: '#ffffff',
        iconSize: 64,
        id: 'hangup_call',
        iconUrl: getStaticS3Asset('icons/off_phone icon.png?v=3'),
        background: [
            new SolidBackground({ color: '#ff0000' }),
        ],
        actions: compact([
            currentData.userId && currentData.callId && {
                log_id: 'video_call_hangup_call_action',
                url: directivesAction(telegramDiscardVideoCallDirective(currentData.userId, currentData.callId)),
            },
        ]),
    });
};

