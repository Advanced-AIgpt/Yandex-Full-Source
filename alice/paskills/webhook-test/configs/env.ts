import { ConfigOverride } from './defaults';

const config: ConfigOverride = {
    api: {
        url: process.env.WEBHOOK_API_URL,
    },
};

export default config;
