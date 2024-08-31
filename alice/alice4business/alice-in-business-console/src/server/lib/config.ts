import * as path from 'path';
import { Config } from '../../configs/defaults';

export { ConfigOverride } from '../../configs/defaults';

process.env.CFG_DIR = path.resolve(__dirname, '../../configs');

const config = require('@yandex-int/yandex-cfg-env') as Config; // tslint:disable-line:no-var-requires
config.client = config.client.bind(config);

export default config;
