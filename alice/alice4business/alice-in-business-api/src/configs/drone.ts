import { ConfigOverride } from './defaults';

const config: ConfigOverride = {
    app: {
        debug: {
            stackTraceSourceMapSupport: true,
            cutInternalStackEntries: true,
        },
        operationsWorker: {
            rejectTimeout: 5000,
        },
        connectSyncWorker: {
            rejectTimeout: 5000,
        },
    },

    db: {
        options: {
            dialectOptions: {
                ssl: false,
            },
            pool: {
                max: 5,
                min: 0,
                idle: 20000,
                acquire: 20000,
            },
        },
    },
};

export default config;
