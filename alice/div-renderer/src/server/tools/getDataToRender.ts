import { IncomingProtobufJSChunk } from '@yandex-int/apphost-lib';
import { logger } from '../../common/logger';
import { NAlice } from '../../protos';
import { putTimeToParseAllProto } from '../../solomon';
import { trackFn } from './trackPerformance';


interface GetDataToRenderParams {
    chunks: IncomingProtobufJSChunk;
    reqLog: typeof logger;
}
export const getDataToRender = trackFn(
    ({ chunks, reqLog }: GetDataToRenderParams) => {
        const renderDataItemList = Array.from(chunks.getItems('render_data'));

        const result = renderDataItemList.map(item => {
            const data = item.parseProto(NAlice.NRenderer.TDivRenderData);

            setTimeout(() => reqLog.debug({ payload: data }, 'Incoming request'));

            return data;
        });

        return result;
    },
    ({ duration }) => putTimeToParseAllProto(duration),
);
