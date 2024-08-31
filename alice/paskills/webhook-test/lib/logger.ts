import createLogger from '@yandex-int/yandex-logger';
import cfg from '../configs';


export default createLogger({
    ...cfg.logger,
    streams: [
        {
            level: 'info',
            stream: require('@yandex-int/yandex-logger/streams/qloud')()
        }
    ]
});
