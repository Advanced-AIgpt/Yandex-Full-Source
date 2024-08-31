import { ConfigOverride } from './defaults';

const testingConfig: ConfigOverride = {
    api: {
        url: 'https://paskills.test.voicetech.yandex.net/b2b',
    },

    tvmtool: {
        host: 'http://localhost:1',
    },
};

module.exports = testingConfig;
