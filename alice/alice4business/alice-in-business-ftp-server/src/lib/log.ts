import kroniko from 'kroniko';

if (process.env.PASKILLS_LOGGER_ENABLED !== 'false') {
    kroniko.install({
        breadcrumbs: {
            http: true,
            trace: true,
        },
        std: {
            format: 'qloud',
            depth: Infinity,
            pretty: true,
            time: true,
        },
    });
}

const log = {
    ...kroniko,
    log: kroniko.log,
    warn: kroniko.warn,
    error: kroniko.error,
    debug: kroniko.debug,
    info: kroniko.info,
    trace: kroniko.trace,
};

export default log;
