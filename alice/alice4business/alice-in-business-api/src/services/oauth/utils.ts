import config from "../../lib/config"
import createGot from "../got"
import merge = require('deepmerge');

const got = createGot({sourceName: 'oauth', baseUrl: config.oauth.host})

export const oauthGot = async <T = any>(path: string, options: object = {}) => {
    const mergedOptions = merge(
        {
            headers: {},
            json: true,
        },
        options,
    );
    return await got(path, mergedOptions).then(({ body }) => (body as unknown) as T);
}