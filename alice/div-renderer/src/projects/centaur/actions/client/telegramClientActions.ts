import { createShowViewClientAction, EnumInactivityTimeout, EnumLayer } from './index';
import { TopLevelCard } from '../../helpers/helpers';
import { BoolIntVariable, ContainerBlock, DivVariable, MatchParentSize } from 'divcard2';
import { createClientAction, IDirectiveClientAction } from '../../../../common/actions/client';
import { TelegramLogin } from '../../divCustoms/TelegramCustoms';
import { createSemanticFrameAction } from '../../../../common/actions/server/index';
import { IRequestState } from '../../../../common/types/common';
import { createRequestState } from '../../../../registries/common';
import { CurrentCallCard, ICurrentCallData } from '../../templates/telegram/CurrentCall';
import { MMRequest } from '../../../../common/helpers/MMRequest';

export const bindToCallVariable: DivVariable = new BoolIntVariable({
    name: 'bindToCall',
    value: 1,
});

export function telegramDiscardVideoCallDirective(
    userId: string,
    callId: string | undefined = undefined,
): IDirectiveClientAction {
    return createClientAction(
        'discard_video_call_directive',
        {
            telegram_discard_video_call_data: {
                call_owner_data: {
                    user_id: userId,
                    call_id: callId,
                },
            },
        },
    );
}

export function telegramAcceptVideoCallDirective(
    callId: string,
    userId: string,
): IDirectiveClientAction {
    return createClientAction(
        'accept_video_call_directive',
        {
            telegram_accept_video_call_data: {
                call_owner_data: {
                    call_id: callId,
                    user_id: userId,
                },
            },
        },
    );
}

export function telegramAcceptVideoCallShowView(
    mmRequest: MMRequest,
    currentData: ICurrentCallData,
): IDirectiveClientAction {
    const actionRequestState = createRequestState(mmRequest);

    return createShowViewClientAction(
        CurrentCallCard(currentData, actionRequestState),
        true,
        EnumLayer.content,
        EnumInactivityTimeout.short,
        'video_call.current_call.action_space_id',
    );
}

export function telegramStartLoginDirective(
    id: string,
): IDirectiveClientAction {
    return createClientAction(
        'start_video_call_login_directive',
        {
            telegram_start_login_data: {
                id,
                on_fail_callback: createSemanticFrameAction(
                    {
                        video_call_login_failed_semantic_frame: {
                            provider: {
                                enum_value: 'Telegram',
                            },
                        },
                    },
                    'VideoCall',
                    'video_call_login_failed',
                ),
            },
        },
    );
}

export function telegramMuteMicDirective(
    userId: string,
    callId: string | undefined = undefined,
): IDirectiveClientAction {
    return createClientAction(
        'video_call_mute_mic_directive',
        {
            telegram_mute_mic_data: {
                call_owner_data: {
                    call_id: callId,
                    user_id: userId,
                },
            },
        },
    );
}

export function telegramUnmuteMicDirective(
    userId: string,
    callId: string | undefined = undefined,
): IDirectiveClientAction {
    return createClientAction(
        'video_call_unmute_mic_directive',
        {
            telegram_unmute_mic_data: {
                call_owner_data: {
                    call_id: callId,
                    user_id: userId,
                },
            },
        },
    );
}

export function turnOnVideoDirective(
    userId: string,
    callId: string | undefined = undefined,
): IDirectiveClientAction {
    return createClientAction(
        'video_call_turn_on_video_directive',
        {
            telegram_turn_on_video_data: {
                call_owner_data: {
                    call_id: callId,
                    user_id: userId,
                },
            },
        },
    );
}

export function turnOffVideoDirective(
    userId: string,
    callId: string | undefined = undefined,
): IDirectiveClientAction {
    return createClientAction(
        'video_call_turn_off_video_directive',
        {
            telegram_turn_off_video_data: {
                call_owner_data: {
                    call_id: callId,
                    user_id: userId,
                },
            },
        },
    );
}

export function telegramStartLoginShowView(
    id: string,
    requestState: IRequestState,
): IDirectiveClientAction {
    return createShowViewClientAction(
        TopLevelCard({
            log_id: 'telegram_login_screen',
            states: [
                {
                    state_id: 0,
                    div: new ContainerBlock({
                        width: new MatchParentSize(),
                        height: new MatchParentSize(),
                        orientation: 'overlap',
                        items: [
                            TelegramLogin(id),
                        ],
                    }),
                },
            ],
        }, requestState),
        false,
        EnumLayer.dialog,
        EnumInactivityTimeout.infinity,
    );
}

