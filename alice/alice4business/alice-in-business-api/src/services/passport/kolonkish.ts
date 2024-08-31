import { passportGotWithTvm } from './utils';
import RestApiError from '../../lib/errors/restApi';
import { getTokenByCode } from '../oauth/token';
import { getCodeForAm } from './code';

export interface KolonkishResponse {
    status: 'ok' | 'error';
    uid: string;
    login: string;
    code: string;
    token?: string;
}

export interface RecreateKolonkishResponse {
    status: 'ok' | 'error';
    code: string;
}

interface KolonkishParams {
    cookie?: string;
    OAuth?: string;
    clientIp?: string;
    displayName?: string;
}

export const recreateKolonkishCode = async (
    {
        OAuth,
        cookie,
        clientIp
    }: KolonkishParams,
    uid: string) => {
    if (!cookie && !OAuth) {
        throw new Error('Ya-Consumer-Authorization or cookie is require');
    }
    return passportGotWithTvm<KolonkishResponse>(
        '/1/bundle/auth/oauth/recreate_kolonkish_token/', {
        method: 'post',
        headers: OAuth
            ? {
                'Ya-Consumer-Authorization': OAuth,
                'Ya-Consumer-Client-Ip': clientIp,
            }
            : {
                'Ya-Consumer-Client-Ip': clientIp,
                'Ya-Client-Cookie': cookie,
                'Ya-Client-Host': 'dialogs.yandex.ru',
            },
        body: { uid },
        form: true,
    }).then(resp => {
        if (resp.status !== 'ok') {
            throw new RestApiError('Failed to recreate kolonkish', 500, {
                payload: { passportResponse: resp },
            });
        }
        return resp.code;
    });
};

export const registryKolonkish = async ({
    OAuth,
    cookie,
    clientIp,
    displayName,
}: KolonkishParams) => {
    if (!cookie && !OAuth) {
        throw new Error('Ya-Consumer-Authorization or cookie is require');
    }

    return passportGotWithTvm<KolonkishResponse>(
        '/1/bundle/account/register/kolonkish/',
        {
            method: 'post',
            headers: OAuth
                ? {
                    'Ya-Consumer-Authorization': OAuth,
                    'Ya-Consumer-Client-Ip': clientIp,
                }
                : {
                    'Ya-Consumer-Client-Ip': clientIp,
                    'Ya-Client-Cookie': cookie,
                    'Ya-Client-Host': 'dialogs.yandex.ru',
                },
            body: { display_name: displayName },
            form: true,
        },
    ).then((resp) => {
        if (resp.status !== 'ok') {
            throw new RestApiError('Failed to register kolonkish', 500, {
                payload: { passportResponse: resp },
            });
        }

        resp.uid = String(resp.uid); // passport returns number here
        return resp;
    });
};


export const multiply = async (baseKolonkish: KolonkishResponse,
    clientIp: string,
    numDevices: number,
    isTv: boolean): Promise<KolonkishResponse[]> => {
    const result: KolonkishResponse[] = [];
    if (!baseKolonkish.token) {
        baseKolonkish.token = await getTokenByCode(baseKolonkish.code);
    }
    for (let i = 0; i < numDevices; i++) {
        const deviceCode = await getCodeForAm({
            OAuth: `OAuth ${baseKolonkish.token}`,
            clientIp,
            isTv
        });
        result.push({status: 'ok', uid: baseKolonkish.uid, login: baseKolonkish.login, code: deviceCode});
    }
    return result;
}