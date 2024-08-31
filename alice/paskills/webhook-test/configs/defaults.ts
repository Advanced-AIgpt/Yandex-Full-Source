import pkg from '../package.json';
import { LoggerOptions } from '@yandex-int/yandex-logger';

const config = {
    api: {
        url: 'https://quiz-backend-ml-2019-test.promo.yandex.ru/test/v1',
    },
    logger: {
        name: pkg.name,
    } as LoggerOptions,
};

export default config;

type RecursivePartial<T> = { [P in keyof T]?: RecursivePartial<T[P]> };

export type Config = typeof config;
export type ConfigOverride = RecursivePartial<Config>;
