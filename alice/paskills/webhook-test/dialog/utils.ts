import { State } from './types';

export const wrapResponse = (
    message: any,
    body: any,
    session_state: State = null,
    user_state_update: State = null,
    analytics: any = null
): any => ({
    response: body,
    session: message.session,
    version: message.version,
    session_state: session_state,
    user_state_update: user_state_update,
    analytics: analytics
});
