import * as yandexLogger from '@yandex-int/yandex-logger';
import * as deployStream from '@yandex-int/yandex-logger/streams/deploy';

import { getEnvs } from '../env';

export const logger = yandexLogger({
    streams: [
        {
            level: getEnvs().isProd ? 'info' : 'debug',
            stream: deployStream(),
        },
    ],
});
