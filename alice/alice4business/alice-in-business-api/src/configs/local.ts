import { ConfigOverride } from './defaults';

const config: ConfigOverride = {
    app: {
        debug: {
            stackTraceSourceMapSupport: true,
            cutInternalStackEntries: true,
        },

        activationCodesCleanupWorker: {
            enable: false,
        },

        operationsWorker: {
            enable: false,
        },

        countersWorker: {
            enable: false,
        },

        quasarSyncWorker: {
            enable: false,
        },

        connectSyncWorker: {
            enable: false,
        },

        resetOperation: {
            switchRetries: 2,
            compareRetries: 3,
        },

        customerActivationOperation: {
            switchRetries: 2,
            compareRetries: 3,
        },
    },

    db: {
        options: {
            dialectOptions: {
                ssl: false,
            },
            // logging: console.log,
            pool: {
                min: 0,
                max: 20,
                acquire: 4500,
                idle: 20000,
                evict: 30000,
            },
        },
        batchSize: 1000000,
    },

    bulbasaur: {
        url: 'http://iot-dev.quasar.yandex.net',
    },

    quasar: {
        url: 'https://quasar.yandex.net',
    },

    connect: {
        defaultHookHost: `${
            process.env.USER || process.env.LOGNAME
        }-10-xproducts.ldev.yandex.ru`,
    },

    dialogovo: {
        mordoviaUrl: `https://${
            process.env.USER || process.env.LOGNAME
        }-20-xproducts.ldev.yandex.ru/b2b/station/`,
    },
};

export default config;
