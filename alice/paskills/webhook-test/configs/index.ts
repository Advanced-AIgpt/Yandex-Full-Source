import { Config as AppConfig } from './defaults';

process.env.CFG_DIR = __dirname;

export interface Config extends AppConfig {
    environment: string;
}

export default require('@yandex-int/yandex-cfg-env').default as Config;
