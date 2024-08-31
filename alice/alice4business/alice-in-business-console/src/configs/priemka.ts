import { ConfigOverride } from './defaults';

const priemkaConfig: ConfigOverride = {
    api: {
        url: 'https://paskills.priemka.voicetech.yandex.net/b2b',
    },

    app: {
        oauthClientId: 'b300a11339614d7a8024c469fa9cf501',
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
        topicPrefix: 'priemka-',
    },

    connect: {
        apiHost: 'https://api-integration-qa.directory.ws.yandex.net',
        frontHost: 'https://connect-integration-qa.ws.yandex.ru',
    },
};

module.exports = priemkaConfig;
