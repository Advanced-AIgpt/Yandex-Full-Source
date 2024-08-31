import Sequelize from 'sequelize';
import * as PG from 'pg';

const config = {
    app: {
        urlRoot: '/api',
        b2bConsolePath: 'https://localhost.msup.yandex.ru:3001/b2b/console',

        debug: {
            stackTraceSourceMapSupport: false,
            cutInternalStackEntries: false,
        },

        agreementCacheTime: 1000 * 60, // 1 minute

        features: {},

        activationCodesCleanupWorker: {
            enable: true,
            interval: 5000,
        },

        operationsWorker: {
            enable: true,
            interval: 500,
            rejectTimeout: 20000,
        },

        countersWorker: {
            enable: true,
            interval: 1000,
        },

        quasarSyncWorker: {
            enable: true,
            interval: 1000,
            batchSize: 100,
        },

        connectSyncWorker: {
            enable: true,
            interval: 1000,
            batchSize: 1,

            minAgeBeforeSync: 1000 * 60 * 30, // 30 minutes
            rejectTimeout: 1000 * 60, // 1 minute
        },

        resetOperation: {
            switchRetries: 3,
            compareRetries: 10,
            compareInterval: 6000,
        },

        customerActivationOperation: {
            codeRefreshInterval: 1000 * 60 * 3, // 3 minutes
            codeTTL: 1000 * 60 * 4, // 4 minutes

            switchRetries: 3,
            compareRetries: 10,
            compareInterval: 6000,
        },

        historyBatchSize: 20,
    },

    blackbox: {
        retries: 1,
        timeout: 500,
        api: 'pass-test.yandex.ru',
    },

    db: {
        uri: 'postgres://minipgaas:nacc6opq@localhost:12000/database',
        options: {
            dialect: 'postgres',
            logging: () => {},
            dialectOptions: {
                keepAlive: true,
                ssl: {
                    rejectUnauthorized: false,
                } as object | false,
            },
            pool: {
                acquire: 4500,
                idle: 20000,
                evict: 30000,
                min: 12,
                max: 20,
                validate: (client?: any) => {
                    if (
                        !client?.connection ||
                        client.connection._invalid ||
                        client.connection._ending
                    ) {
                        return false;
                    }

                    if (!client.connection._masterValidationPromise) {
                        client.connection._masterValidationPromise = (client as PG.Client)
                            .query('SELECT pg_is_in_recovery();')
                            .then((res) => {
                                if (res?.rows?.[0]?.pg_is_in_recovery !== false) {
                                    client.destroy();
                                }
                            })
                            .finally(() => {
                                client.connection._masterValidationPromise = null;
                            });
                    }

                    return true;
                },
            },
            retry: {
                timeout: 5000,
                name: 'Query',
            },
        } as Sequelize.Options,
        batchSize: 1000,
    },

    tvmtool: {
        host: 'http://localhost:9999',
        token: '5832513925e456dcdb8a0b1c5d16ed11',
        selfAlias: 'alice4business',
        selfId: 2015559,
    },

    server: {
        timeout: 120000,
    },

    terminus: {
        enabled: false,
        healthCheckRoute: '/ping',
        signal: 'SIGUSR2',
        timeout: 1000,
        beforeShutdownTimeout: 0,
        exitOnShutdown: true,
    },

    quasar: {
        url: 'https://quasar.yandex.net/dev',
        retryInterval: 20000,
        timeout: 10000,

        subscriptionStatus: {
            activationShiftMs: 30 * 60 * 1000, // now() + 30 minutes
            deactivationShiftMs: -1000, // now() - 1 second
        },

        auxiliaryConfig: {
            lifetimeMs: 15 * 60 * 1000, // 15 minutes
        },

        oauthClient: {
            id: '',
            secret: '',
        },

        oauthClientTV: {
            id: '',
            secret: '',
        },
    },

    passport: {
        host: 'https://passport-test-internal.yandex.ru',
        consumer: 'alicebusiness',
        timeout: 5000,
    },

    oauth: {
        host: 'https://oauth.yandex.ru'
    },

    // Для выдачи промокодов
    mediabilling: {
        url: 'https://api.mt.mediabilling.yandex.net',
        tvmId: 2001265,
        retries: 20,
    },

    bulbasaur: {
        url: 'http://iot.quasar.yandex.net',
    },

    push: {
        host: 'https://push-sandbox.yandex.ru',
        token: '',
        topicPrefix: 'dev-',
    },

    connect: {
        url: 'https://api-internal-test.directory.ws.yandex.net',
        selfSlug: 'alice_b2b',
        techDomainSuffix: '.yaconnect.com',
        tvmId: 2000204,
        defaultHookHost: '$HOSTNAME',
    },

    dialogovo: {
        tvmId: [2015559, 2015307, 2015309], // self, dialogovo.testing, dialogovo.staging
        mordoviaUrl: 'https://localhost.msup.yandex.ru:3001/b2b/station/',
    },

    droideka: {
        tvmId: [2015559, 2018594, 2018019, 2018021], // self, droideka.dev, droideka.testing, droideka.prod
    },

    idm: {
        tvmId: [2015559], // self
    },
};

type RecursivePartial<T> = { [P in keyof T]?: RecursivePartial<T[P]> };

export type Config = typeof config;
export type ConfigOverride = RecursivePartial<Config>;

export default config;
