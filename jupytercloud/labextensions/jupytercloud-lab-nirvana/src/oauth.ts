import { openPopup } from './popup';

export interface IOauthResult {
    success: boolean;
    message?: string;
}

type TOAuthCallback = (result: IOauthResult) => void;

export const OAuthPopup = (url: string, callback: TOAuthCallback): void => {
    const popupRef = openPopup('jcne-oauth-popup', url);

    if (!popupRef || popupRef.closed) {
        callback({ success: false, message: 'fail to open popup' });
        return;
    }

    let resolved = false;

    const onMessage = (event: MessageEvent) => {
        const data = event.data;
        if (
            data &&
            data.hasOwnProperty('event') &&
            data.event === 'jupytercloud-oauth'
        ) {
            window.removeEventListener('message', onMessage);

            const oauthResult: IOauthResult = {
                success: data.success
            };
            if (data.message) {
                oauthResult.message = data.message;
            }

            resolved = true;
            callback(oauthResult);
        }
    };

    // in case popup already open
    window.removeEventListener('message', onMessage);

    window.addEventListener('message', onMessage, false);

    popupRef.onunload = () => {
        // onunload somewhy happens before recieving message from popup page
        // so we wait one second to conclude that page was closed without message
        setTimeout(() => {
            if (resolved) {
                return;
            }

            const oauthResult: IOauthResult = {
                success: false,
                message: 'popup was manually closed'
            };

            callback(oauthResult);
        }, 2000);
    };
};
