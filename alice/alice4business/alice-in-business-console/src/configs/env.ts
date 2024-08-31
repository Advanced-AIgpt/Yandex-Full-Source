import { ConfigOverride } from './defaults';

function env<T extends string = string>(key: string) {
    return process.env[key] as T | undefined;
}

env.bool = cast((str: string) => str !== 'false');
env.number = cast(Number);
env.json = cast(JSON.parse);

function cast<T>(fn: (val: string) => T) {
    return (key: string) => {
        const val = env(key);

        return val === undefined ? val : fn(val);
    };
}

const envConfig: ConfigOverride = {
    api: {
        url: env('PASKILLS_B2B_API_URL'),
    },
    app: {
        urlRoot: env('PASKILLS_B2B_URL_ROOT'),
        checkCsrf: env.bool('PASKILLS_B2B_CRFS_CHECK'),
        oauthClientId: env('PASKILLS_B2B_OAUTH_CLIENT_ID'),
        pollInterval: env.number('PASKILLS_B2B_POLL_INTERVAL'),
        features: {
            history: env.bool('PASKILLS_B2B_FEATURES_HISTORY'),
            settings: env.bool('PASKILLS_B2B_SETTINGS_HISTORY'),
        },
    },
    passport: {
        host: env('PASSPORT_API'),
    },

    blackbox: {
        api: env('BLACKBOX_API'),
    },

    tvmtool: {
        host: env('TVMTOOL_HOST') || env('DEPLOY_TVM_TOOL_URL'),
        token: env('TVMTOOL_LOCAL_AUTHTOKEN') || env('QLOUD_TVM_TOKEN'),
    },

    push: {
        host: env('PASKILLS_PUSH_HOST'),
        topicPrefix: env('PASKILLS_PUSH_TOPIC_PREFIX'),
    },

    connect: {
        apiHost: env('PASKILLS_CONNECT_API_HOST'),
        frontHost: env('PASKILLS_CONNECT_FRONT_HOST'),
    },
};

module.exports = envConfig;
