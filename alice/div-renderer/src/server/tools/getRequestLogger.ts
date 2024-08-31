import { IncomingProtobufJSChunk } from '@yandex-int/apphost-lib';
import { logger } from '../../common/logger';

export const getRequestLogger = (chunks: IncomingProtobufJSChunk) => {
    const { reqid } = chunks.getOnlyItem('app_host_params').parseJSON() as { reqid: string };
    const reqLog = logger.child({ requestId: reqid });

    return reqLog;
};
