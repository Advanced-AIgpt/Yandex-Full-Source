import { ConfigOverride } from './defaults';

process.env.NODE_TLS_REJECT_UNAUTHORIZED = '0';

const developmentConfig: ConfigOverride = {
    app: {
        log: {
            breadcrumbs: {
                http: true,
            },
            std: {
                pretty: true,
            },
        },

        activationCodeRefreshInterval: 10000,
    },

    tvmtool: {
        host: 'http://localhost:9999',
        token: '5832513925e456dcdb8a0b1c5d16ed11', // See paskills/api/package.json->scripts->dev:tvmtool
    },
};

module.exports = developmentConfig;
