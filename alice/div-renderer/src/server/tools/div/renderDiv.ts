import { MMRequest } from '../../../common/helpers/MMRequest';
import { logger } from '../../../common/logger';
import { NAlice } from '../../../protos';
import { createRequestState } from '../../../registries/common';
import { putTimeToRender } from '../../../solomon';
import { trackFn } from '../trackPerformance';
import { tryGetScenarioMeta } from '../tryGetScenarioMeta';
import { tryGetDivTemplateFn } from './tryGetDivTemplateFn';
import { tryRenderDivTemplate } from './tryRenderDivTemplate';

interface RenderDivParams {
    data: NAlice.NRenderer.TDivRenderData;
    mmRequest: MMRequest;
    reqLog: typeof logger;
}
export const renderDiv = trackFn(({ data, mmRequest, reqLog }: RenderDivParams) => {
    const requestState = createRequestState(mmRequest);
    const { templateData, templateId } = tryGetScenarioMeta({ data, reqLog });
    const templateFn = tryGetDivTemplateFn({ reqLog, requestState, data, templateId });

    if (!templateFn) {
        return undefined;
    }

    const result = tryRenderDivTemplate({
        templateData,
        templateId,
        templateFn,
        requestState,
        mmRequest,
        reqLog,
    });

    return { result, requestState, data, templateId };
}, ({ duration }) => putTimeToRender(duration));
