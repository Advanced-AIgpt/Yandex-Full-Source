import createGot from '../got';
import config from '../../lib/config';
import merge = require('deepmerge');

const got = createGot({ sourceName: 'push', baseUrl: config.push.host });

export const pushGot = async <T = any>(path: string, options?: any) => {
    if (!config.push.token) {
        throw new Error('Push token is required');
    }

    const merged = merge(
        {
            headers: {
                Authorization: `Xiva ${config.push.token}`,
            },
            json: true,
        },
        options,
    );
    return got(path, merged).then(({ body }) => (body as unknown) as T);
};
