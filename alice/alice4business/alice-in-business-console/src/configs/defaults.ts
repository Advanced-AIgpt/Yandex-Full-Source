import url from 'url';

const defaultConfig = {
    api: {
        url: 'http://localhost.msup.yandex.ru:7080/api',
    },
    app: {
        urlRoot: '/b2b',
        assetsRoot: '/b2b',

        apiProxy: '/b2b/api',
        apiTimeout: 30000,

        apiSupportProxy: '/b2b/api/support',
        apiPublicProxy: '/b2b/api/public',
        oauthClientId: '848b4560b4464f06a6a30dd2a992d8a7',
        checkCsrf: true,
        pollInterval: 1500,

        features: {
            history: true,
            settings: true,
        },

        log: {
            breadcrumbs: {
                http: false,
                trace: true,
            },
            std: {
                format: 'qloud',
                depth: Infinity,
                pretty: false,
                time: true,
            },
        },

        activationCodeRefreshInterval: 30000,
    },

    blackbox: {
        retries: 1,
        timeout: 500,
        api: 'pass-test.yandex.ru',
        attributes: {
            login: '1008',
            connect: '1017',
            havePlus: '1015',
        },
        multisession: 'yes' as 'yes',
    },

    passport: {
        protocol: 'https',
        host: 'passport-test.yandex.ru',
    },

    passportAccounts: {
        protocol: 'https',
        host: 'api.passport-test.yandex.ru',
        pathname: '/all_accounts',
    },

    avatar: {
        protocol: 'https',
        host: 'avatars.mdst.yandex.net',
    },

    tvmtool: {
        host: 'http://localhost:9999',
        token: '5832513925e456dcdb8a0b1c5d16ed11', // See paskills/api/package.json->scripts->dev:tvmtool,
        src: 'alice4business',
    },

    push: {
        host: 'wss://push-sandbox.yandex.ru',
        service: 'alice-b2b',
        topicPrefix: 'dev-',
    },

    connect: {
        apiHost: 'https://api-internal-test.directory.ws.yandex.net',
        frontHost: 'https://connect-test.ws.yandex.ru',
    },

    client() {
        return {
            features: this.app.features,

            apiRoot: this.app.apiProxy,
            pollTimeout: this.app.pollInterval,
            urlRoot: this.app.urlRoot,
            assetsRoot: this.app.assetsRoot,
            passportHost: url.format(this.passport),
            passportAccounts: url.format(this.passportAccounts),
            avatarHost: url.format(this.avatar),
            connectHost: this.connect.frontHost,
            xiva: this.push,
        };
    },
};

type RecursivePartial<T> = { [P in keyof T]?: RecursivePartial<T[P]> };

export type Config = typeof defaultConfig;
export type ConfigOverride = RecursivePartial<Config>;

module.exports = defaultConfig;
