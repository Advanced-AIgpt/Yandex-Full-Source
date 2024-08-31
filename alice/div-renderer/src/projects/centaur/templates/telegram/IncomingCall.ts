import {
    FixedSize,
    TextBlock,
    SolidBackground,
} from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { title36m, title48m } from '../../style/Text/Text';
import { TopLevelCard } from '../../helpers/helpers';
import {
    telegramDiscardVideoCallDirective,
    telegramAcceptVideoCallDirective,
    telegramAcceptVideoCallShowView,
    bindToCallVariable,
} from '../../actions/client/telegramClientActions';
import { closeLayerAction } from '../../actions/client/index';
import { Layer } from '../../common/layers';
import { directivesAction } from '../../../../common/actions';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { DualScreen } from '../../components/DualScreen/DualScreen';
import PartImage from '../../components/DualScreen/partComponents/PartImage';
import getColorSet, { IColorSet } from '../../style/colorSet';
import PartBasicTopCenterBottom from '../../components/DualScreen/partComponents/PartBasicTopCenterBottom';
import { IconButton } from '../../../../common/components/IconButton';
import { offsetFromEdgeOfScreen } from '../../style/constants';
import { getStaticS3Asset } from '../../helpers/assets';

export const DeviceName = 'SmartSpeaker';

export default function IncomingCall(
    incomingData: NAlice.NData.ITIncomingTelegramCallData,
    mmRequest: MMRequest,
    requestState: IRequestState,
) {
    const colorSet = getColorSet();

    requestState.variables.add(bindToCallVariable);

    return TopLevelCard({
        log_id: 'incoming_call_card',
        states: [
            {
                state_id: 0,
                div: DualScreen({
                    requestState,
                    firstDiv: PartBasicTopCenterBottom({
                        topSize: new FixedSize({ value: 300 }),
                        bottomSize: new FixedSize({ value: 300 }),
                        middleDivItems: [
                            new TextBlock({
                                ...title36m,
                                text_color: colorSet.textColorOpacity50,
                                text: 'Telegram',
                                text_alignment_horizontal: 'center',
                            }),
                            new TextBlock({
                                ...title48m,
                                text_color: colorSet.textColor,
                                text: incomingData.Caller?.TelegramContactData?.DisplayName || ' ',
                                text_alignment_horizontal: 'center',
                            }),
                        ],
                        bottomDivItems: [
                            discardCallButton(incomingData, colorSet),
                            acceptCallButton(mmRequest, incomingData, colorSet),
                        ],
                        bottomDivOptions: {
                            orientation: 'horizontal',
                            content_alignment_horizontal: 'center',
                        },
                    }),
                    secondDiv: PartImage({
                        imageUrl: 'df',
                        imageOptions: {
                            extensions: [
                                {
                                    id: 'telegram-avatar',
                                    params: {
                                        user_id: incomingData.Caller?.TelegramContactData?.UserId,
                                    },
                                },
                            ],
                        },
                    }),
                    mainColor1: colorSet.mainColor1,
                    mainColor: colorSet.mainColor,
                }),
            },
        ],
    }, requestState);
}

const acceptCallButton = (
    mmRequest: MMRequest,
    incomingData: NAlice.NData.ITIncomingTelegramCallData,
    colorSet: IColorSet,
) => {
    return IconButton({
        size: 112,
        color: colorSet.mainColor,
        iconSize: 64,
        id: 'accept_call',
        iconUrl: getStaticS3Asset('icons/on_phone icon.png?v=3'),
        background: [
            new SolidBackground({ color: colorSet.textColor }),
        ],
        margins: {
            bottom: offsetFromEdgeOfScreen + 20,
        },
        actions: compact([
            incomingData.CallId && incomingData.UserId && {
                log_id: 'video_call_accept_call_actions',
                url: directivesAction([
                    telegramAcceptVideoCallDirective(incomingData.CallId, incomingData.UserId),
                    telegramAcceptVideoCallShowView(
                        mmRequest,
                        {
                            userId: incomingData.UserId,
                            callId: incomingData.CallId,
                            recipient: {
                                userId: incomingData.Caller!!.TelegramContactData!!.UserId!!,
                                displayName: incomingData.Caller!!.TelegramContactData!!.DisplayName || ' ',
                            },
                        }),
                ]),
            },
            closeLayerAction('close_incoming_call_layer', Layer.ALARM, false),
        ]),
    });
};

const discardCallButton = (
    incomingData: NAlice.NData.ITIncomingTelegramCallData,
    colorSet: IColorSet,
) => {
    return IconButton({
        size: 112,
        color: colorSet.mainColor,
        iconSize: 64,
        id: 'discard_call',
        iconUrl: getStaticS3Asset('icons/off_phone icon.png?v=3'),
        background: [
            new SolidBackground({ color: colorSet.textColorOpacity20 }),
        ],
        margins: {
            bottom: offsetFromEdgeOfScreen + 20,
            right: 32,
        },
        actions: compact([
            incomingData.CallId && incomingData.UserId && {
                log_id: 'video_call_discard_call_action',
                url: directivesAction(telegramDiscardVideoCallDirective(incomingData.UserId, incomingData.CallId)),
            },
            closeLayerAction('close_incoming_call_layer', Layer.ALARM, false),
        ]),
    });
};
