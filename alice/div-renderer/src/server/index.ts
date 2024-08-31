import { Service, ServiceTransport } from '@yandex-int/apphost-lib';
import { logger } from '../common/logger';
import { config } from './config';
import {
    startSolomon,
    putTotalTime,
    putTotalTimeBatchSizeOne,
} from '../solomon';
import { startHandlePerformanceMetrics } from './tools/metrics/performance';
import { getEnvs } from '../env';
import { getTimeMark } from './tools/trackPerformance';
import { getRequestLogger } from './tools/getRequestLogger';
import { createMMRequest } from './tools/createMMRequest';
import { getDataToRender } from './tools/getDataToRender';
import { encodeDivResult } from './tools/div/encodeDivResult';
import { encodeDivPatchResult } from './tools/divPatch/encodeDivPatchResult';
import { renderDiv } from './tools/div/renderDiv';
import { renderDivPatch } from './tools/divPatch/renderDivPatch';
import { getChunks } from './tools/getChunks';
import { sendResult } from './tools/sendResult';
import { NAlice } from '../protos';
import { compact } from 'lodash';
type TDivRenderData = NAlice.NRenderer.TDivRenderData;

const serv = Service.create({ transport: ServiceTransport.NEH });

const isDivPatch = (data: TDivRenderData) => {
    if (data.ScenarioData) {
        return false;
    }
    return true;
};

serv
    .addHandler('/render', async context => {
        if (getEnvs().isDev) {
            // eslint-disable-next-line no-console
            console.profile('render');
        }

        const renderStartMark = getTimeMark();

        const chunks = await getChunks(context);

        const reqLog = getRequestLogger(chunks);

        const mmRequest = createMMRequest(chunks);

        const renderChunksStartMark = getTimeMark();

        const dataToRender = getDataToRender({ chunks, reqLog });

        const protoResult = (() => {
            return compact(dataToRender.map(data => {
                if (isDivPatch(data)) {
                    const renderDivPatchResult = renderDivPatch({ data, mmRequest, reqLog });

                    return encodeDivPatchResult(renderDivPatchResult);
                } else {
                    const renderDivResult = renderDiv({ data, mmRequest, reqLog });

                    if (!renderDivResult?.data) {
                        return undefined;
                    }

                    return encodeDivResult(renderDivResult);
                }
            }));
        })();

        sendResult({ context, protoResult });

        // шлем метрики продолжительности рендера чанков
        const chunksRenderDuration = renderChunksStartMark();
        if (chunksRenderDuration > config.totalSlowRequest) {
            setTimeout(() => reqLog.warn({ chunksRenderDuration }, 'Rendering request[phase: render-only] took long'));
        }

        // шлем метрики продолжительности общего рендера
        const renderDuration = renderStartMark();
        if (renderDuration > config.totalSlowRequest) {
            reqLog.warn({ renderDuration }, 'Rendering request[phase: total] took long');
        }

        putTotalTime(renderDuration);

        const batchSize = dataToRender.length;
        if (batchSize === 1) {
            putTotalTimeBatchSizeOne(renderDuration);
        }

        reqLog.info({ renderDuration }, 'Rendering request finished successfully');

        if (getEnvs().isDev) {
            // eslint-disable-next-line no-console
            console.profileEnd('render');
        }
    })
    .addHandler('/ping', async() => {
    })
    .listen(getEnvs().PORT);

// region Мягкое выключение для быстрой перезагрузки
function gracefulShutdown(callback: () => void) {
    serv.stop()
        .then(() => callback());
}

process.once('SIGTERM', function() {
    gracefulShutdown(function() {
        process.kill(process.pid, 'SIGTERM');
    });
});

logger.info(`Service started and listening on ${getEnvs().PORT}`);

startHandlePerformanceMetrics();
startSolomon();
