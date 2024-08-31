import { ConfigOverride } from './defaults';

const stableConfig: ConfigOverride = {
    api: {
        url: 'https://paskills.voicetech.yandex.net/b2b',
    },
    app: {
        oauthClientId: 'b300a11339614d7a8024c469fa9cf501',
        features: {
            settings: false,
        },
    },

    blackbox: {
        api: 'blackbox.yandex.net',
    },

    passport: {
        host: 'passport.yandex.ru',
    },

    passportAccounts: {
        host: 'api.passport.yandex.ru',
    },

    avatar: {
        host: 'avatars.mds.yandex.net',
    },

    tvmtool: {
        host: 'http://localhost:1',
    },

    push: {
        host: 'wss://push.yandex.ru',
        topicPrefix: '',
    },

    connect: {
        apiHost: 'https://api-internal.directory.ws.yandex.net',
        frontHost: 'https://connect.yandex.ru',
    },
};

module.exports = stableConfig;
