import { DivCustomBlock, MatchParentSize } from 'divcard2';

export function TelegramCall(callId: String) {
    return new DivCustomBlock({
        custom_type: 'telegram_call',
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        custom_props: {
            id: callId,
        },
        extensions: [
            {
                id: 'telegram-call',
            },
        ],
    });
}

export function TelegramLogin(id: String) {
    return new DivCustomBlock({
        custom_type: 'telegram_login',
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        custom_props: {
            id,
        },
        extensions: [
            {
                id: 'telegram-login',
            },
        ],
    });
}
