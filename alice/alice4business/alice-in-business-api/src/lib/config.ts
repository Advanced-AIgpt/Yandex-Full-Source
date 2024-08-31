import dotenv from 'dotenv';
import path from 'path';
import { Config as AppConfig } from '../configs/defaults';

process.env.CFG_DIR = path.resolve(__dirname, '../configs');

try {
    dotenv.config();
} catch (e) {
    // Ignore error
}

export interface Config extends AppConfig {
    environment: string;
}

export default require('yandex-cfg-env').default as Config; // tslint:disable-line:no-var-requires

export { ConfigOverride } from '../configs/defaults';
