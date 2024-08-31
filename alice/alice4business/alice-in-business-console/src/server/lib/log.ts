import log from 'kroniko';
import config from './config';

log.install(config.app.log);

export default log;
