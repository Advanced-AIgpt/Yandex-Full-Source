import config from '../lib/config';

delete config.db.options.pool;
delete config.db.options.retry;

export = {
    url: config.db.uri,
    ...config.db.options,
};
