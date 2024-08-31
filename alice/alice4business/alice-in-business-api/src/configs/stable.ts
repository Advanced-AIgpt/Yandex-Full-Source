import { ConfigOverride } from './defaults';

const config: ConfigOverride = {
    app: {
        agreementCacheTime: 1000 * 60 * 60, // 1 hour
    },

    db: {
        options: {
            logging: false,
            dialectOptions: {
                ssl: {
                    ca: require('fs').readFileSync('/root/.postgresql/root.crt', 'utf8'),
                    rejectUnauthorized: true,
                } as object | false,
            },
        },
    },

    blackbox: {
        api: 'blackbox.yandex.net',
    },

    tvmtool: {
        host: 'http://localhost:1',
        selfId: 2015561,
    },

    passport: {
        host: 'https://passport-internal.yandex.ru',
    },

    quasar: {
        url: 'https://quasar.yandex.net',
    },

    mediabilling: {
        url: 'https://api.mediabilling.yandex.net',
        tvmId: 2001267,
    },

    push: {
        host: 'https://push.yandex.ru',
        topicPrefix: '',
    },

    connect: {
        url: 'https://api-internal.directory.ws.yandex.net',
        tvmId: 2000205,
        defaultHookHost: 'paskills.voicetech.yandex.net',
    },

    dialogovo: {
        tvmId: [2015309], // dialogovo.production
        mordoviaUrl: 'https://dialogs.yandex.ru/b2b/station/',
    },

    droideka: {
        tvmId: [2018021], // droideka prod
    },

    idm: {
        tvmId: [2001600], // idm.prod
    },
};

export default config;
