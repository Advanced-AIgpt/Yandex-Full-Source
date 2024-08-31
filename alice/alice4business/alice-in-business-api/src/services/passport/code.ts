import { passportGotWithTvm } from './utils';
import RestApiError from '../../lib/errors/restApi';
import config from '../../lib/config';

export interface CodeForAmResponse {
    status: 'ok' | 'error';
    code: string;
}

interface CodeForAmParams {
    cookie?: string;
    OAuth?: string;
    clientIp?: string;
    isTv: boolean;
}

export const getCodeForAm = async ({ OAuth, cookie, clientIp, isTv }: CodeForAmParams) => {
    if (!cookie && !OAuth) {
        throw new Error('Ya-Consumer-Authorization or cookie is require');
    }

    // TODO: https://st.yandex-team.ru/PASKILLS-4703#5e67672bd6a8cc01fd3bb437
    return passportGotWithTvm<CodeForAmResponse>('/1/bundle/auth/oauth/code_for_am/', {
        method: 'post',
        headers: {
            'Ya-Consumer-Client-Scheme': 'https',
            'Ya-Consumer-Client-Ip': clientIp,
            ...(OAuth
                ? {
                      'Ya-Consumer-Authorization': OAuth,
                  }
                : {
                      'Ya-Client-Cookie': cookie,
                      'Ya-Client-Host': 'dialogs.yandex.ru',
                  }),
        },
        body: {
            client_id: isTv ? config.quasar.oauthClientTV.id : config.quasar.oauthClient.id,
            client_secret: isTv ? config.quasar.oauthClientTV.secret : config.quasar.oauthClient.secret,
        },
        form: true,
    }).then((resp) => {
        if (resp.status !== 'ok') {
            throw new RestApiError('Failed to issue authorization code', 500, {
                payload: { passportResponse: resp },
            });
        }

        return resp.code;
    });
};
