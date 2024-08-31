import { ConfigOverride } from './defaults';

const config: ConfigOverride = {
    db: {
        options: {
            dialectOptions: {
                ssl: {
                    ca: require('fs').readFileSync('/root/.postgresql/root.crt', 'utf8'),
                    rejectUnauthorized: true,
                } as object | false,
            },
        },
    },
    tvmtool: {
        host: 'http://localhost:1',
        selfId: 2015561,
    },
    terminus: {
        enabled: true,
        timeout: 5000,
    },

    blackbox: {
        api: 'blackbox.yandex.net',
    },

    passport: {
        host: 'https://passport-internal.yandex.ru',
    },

    quasar: {
        url: 'https://quasar.yandex.net',
    },

    push: {
        host: 'https://push.yandex.ru',
        topicPrefix: 'priemka-',
    },

    connect: {
        url: 'https://api-integration-qa.directory.ws.yandex.net',
        tvmId: 2000205,
        defaultHookHost: 'paskills.priemka.voicetech.yandex.net',
    },

    dialogovo: {
        tvmId: [2015307, 2015309], // dialogovo.testing, dialogovo.production
        mordoviaUrl: 'https://priemka.dialogs.alice.yandex.ru/b2b/station/',
    },

    idm: {
        tvmId: [2001602, 2001600], // idm.testing, idm.prod
    },
};

export default config;
