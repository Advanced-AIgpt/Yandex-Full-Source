import { RetryOptions } from 'sequelize';
import { ConfigOverride } from './defaults';

function env<T extends string = string>(key: string) {
    return process.env[key] as T | undefined;
}

function cast<T>(fn: (val: string) => T) {
    return (key: string) => {
        const val = env(key);

        return val === undefined ? val : fn(val);
    };
}

const bool = (str: string) => str.toLowerCase() !== 'false';

env.bool = cast(bool);
env.number = cast(Number);
env.numberArray = cast((val) =>
    val
        .split(/[^0-9]+/)
        .map((item) => parseInt(item, 10))
        .filter((item) => !isNaN(item)),
);
env.json = cast(JSON.parse);

const config: ConfigOverride = {
    app: {
        urlRoot: env('PASKILLS_URL_ROOT'),

        debug: {
            stackTraceSourceMapSupport: env.bool('PASKILLS_APP_DEBUG_STACK_SOURCE_MAPS'),
            cutInternalStackEntries: env.bool('PASKILLS_APP_DEBUG_CUT_STACK_INTERNALS'),
        },

        agreementCacheTime: env.number('PASKILLS_APP_AGREEMENT_CACHE_TIME'),

        features: {},

        activationCodesCleanupWorker: {
            enable: env.bool('PASKILLS_APP_ACTIVATION_CODE_CLEANUP_WORKER_ENABLE'),
            interval: env.number('PASKILLS_APP_ACTIVATION_CODE_CLEANUP_WORKER_INTERVAL'),
        },

        operationsWorker: {
            enable: env.bool('PASKILLS_APP_OPERATIONS_WORKER_ENABLE'),
            interval: env.number('PASKILLS_APP_OPERATIONS_WORKER_INTERVAL'),
            rejectTimeout: env.number('PASKILLS_APP_OPERATIONS_WORKER_REJECT_TIMEOUT'),
        },

        countersWorker: {
            enable: env.bool('PASKILLS_APP_COUNTERS_WORKER_ENABLE'),
            interval: env.number('PASKILLS_APP_COUNTERS_WORKER_INTERVAL'),
        },

        quasarSyncWorker: {
            enable: env.bool('PASKILLS_APP_QUASAR_SYNC_WORKER_ENABLE'),
            interval: env.number('PASKILLS_APP_QUASAR_SYNC_WORKER_INTERVAL'),
        },

        connectSyncWorker: {
            enable: env.bool('PASKILLS_APP_CONNECT_SYNC_WORKER_ENABLE'),
            interval: env.number('PASKILLS_APP_CONNECT_SYNC_WORKER_INTERVAL'),
            batchSize: env.number('PASKILLS_APP_CONNECT_SYNC_WORKER_BATCH_SIZE'),

            minAgeBeforeSync: env.number('PASKILLS_APP_CONNECT_SYNC_WORKER_MIN_AGE_MS'),
            rejectTimeout: env.number(
                'PASKILLS_APP_CONNECT_SYNC_WORKER_REJECT_TIMEOUT_MS',
            ),
        },

        resetOperation: {
            switchRetries: env.number('PASKILLS_APP_RESET_OPERATION_SWITCH_RETRIES'),
            compareRetries: env.number('PASKILLS_APP_RESET_OPERATION_COMPARE_RETRIES'),
            compareInterval: env.number('PASKILLS_APP_RESET_OPERATION_COMPARE_INTERVAL'),
        },

        customerActivationOperation: {
            codeRefreshInterval: env.number(
                'PASKILLS_APP_CUSTOMER_ACTIVATION_OPERATION_CODE_REFRESH_INTERVAL_MS',
            ),
            codeTTL: env.number('PASKILLS_APP_CUSTOMER_ACTIVATION_OPERATION_CODE_TTL_MS'),

            switchRetries: env.number(
                'PASKILLS_APP_CUSTOMER_ACTIVATION_OPERATION_SWITCH_RETRIES',
            ),
            compareRetries: env.number(
                'PASKILLS_APP_CUSTOMER_ACTIVATION_OPERATION_COMPARE_RETRIES',
            ),
            compareInterval: env.number(
                'PASKILLS_APP_CUSTOMER_ACTIVATION_OPERATION_COMPARE_INTERVAL',
            ),
        },
    },

    blackbox: {
        api: env('PASKILLS_BLACKBOX_API'),
    },

    db: {
        uri: env('PASKILLS_DB_URI'),
        options: {
            retry: {
                timeout: env.number('PASKILLS_DB_TIMEOUT'),
            } as RetryOptions,
        },
    },

    tvmtool: {
        host: env('PASKILLS_QLOUD_TVM_HOST') || env('DEPLOY_TVM_TOOL_URL'),
        token: env('TVMTOOL_LOCAL_AUTHTOKEN') || env('QLOUD_TVM_TOKEN'),

        selfAlias: env('PASKILLS_TVM_SELF_ALIAS'),
        selfId: env.number('PASKILLS_TVM_SELF_ID'),
    },

    server: {
        timeout: env.number('PASKILLS_SERVER_TIMEOUT'),
    },

    terminus: {
        enabled: env.bool('PASKILLS_TERMINUS_ENABLED'),
        healthCheckRoute: env('PASKILLS_TERMINUS_HEALTH_CHECK_ROUTE'),
        signal: env('PASKILLS_TERMINUS_SIGNAL'),
        timeout: env.number('PASKILLS_TERMINUS_TIMEOUT'),
        beforeShutdownTimeout: env.number('PASKILLS_TERMINUS_BEFORE_SHUTDOWN_TIMEOUT'),
        exitOnShutdown: env.bool('PASKILLS_TERMINUS_EXIT_ON_SHUTDOWN'),
    },

    passport: {
        host: env('PASKILLS_PASSPORT_HOST'),
        consumer: env('PASKILLS_PASSPORT_CONSUMER'),
    },

    quasar: {
        url: env('PASKILLS_QUASAR_URL'),
        retryInterval: env.number('PASKILLS_QUASAR_RETRY_INTERVAL'),
        timeout: env.number('PASKILLS_QUASAR_TIMEOUT'),

        subscriptionStatus: {
            activationShiftMs: env.number(
                'PASKILLS_QUASAR_SUB_STATUS_ACTIVATION_SHIFT_MS',
            ),
            deactivationShiftMs: env.number(
                'PASKILLS_QUASAR_SUB_STATUS_DEACTIVATION_SHIFT_MS',
            ),
        },

        auxiliaryConfig: {
            lifetimeMs: env.number('PASKILLS_QUASAR_AUX_CONFIG_LIFETIME_MS'),
        },

        oauthClient: {
            id: env('PASKILLS_QUASAR_OAUTH_CLIENT_ID'),
            secret: env('PASKILLS_QUASAR_OAUTH_CLIENT_SECRET'),
        },

        oauthClientTV: {
            id: env('PASKILLS_QUASAR_OAUTH_TV_CLIENT_ID'),
            secret: env('PASKILLS_QUASAR_OAUTH_TV_CLIENT_SECRET'),
        }
    },

    bulbasaur: {
        url: env('PASKILLS_BULBASAUR_URL'),
    },

    push: {
        host: env('PASKILLS_PUSH_HOST'),
        token: env('PASKILLS_PUSH_TOKEN'),
        topicPrefix: env('PASKILLS_PUSH_TOPIC_PREFIX'),
    },

    connect: {
        url: env('PASKILLS_CONNECT_URL'),
        selfSlug: env('PASKILLS_CONNECT_SELF_SLUG'),
        techDomainSuffix: env('PASKILLS_CONNECT_TECH_DOMAIN'),
        tvmId: env.number('PASKILLS_CONNECT_TVM_ID'),
        defaultHookHost: env('PASKILLS_CONNECT_DEFAULT_HOOK_HOST'),
    },

    dialogovo: {
        tvmId: env.numberArray('PASKILLS_DIALOGOVO_TVM_ID'),
        mordoviaUrl: env('PASKILLS_DIALOGOVO_MORDOVIA_URL'),
    },
};

export default config;
