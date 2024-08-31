import { MMRequest } from '../../../common/helpers/MMRequest';
import { logger } from '../../../common/logger';
import { NAlice } from '../../../protos';
import { createRequestState } from '../../../registries/common';
import { tryGetScenarioMeta } from '../tryGetScenarioMeta';
import { tryGetDivPatchTemplateFn } from './tryGetDivPatchTemplateFn';
import { tryRenderDivPatchTemplate } from './tryRenderDivPatchTemplate';

interface RenderDivPatchParams {
    data: NAlice.NRenderer.TDivRenderData;
    mmRequest: MMRequest;
    reqLog: typeof logger;
}

export const renderDivPatch = ({ data, mmRequest, reqLog }: RenderDivPatchParams) => {
    const requestState = createRequestState(mmRequest);
    const { templateData, templateId } = tryGetScenarioMeta({ data, reqLog });
    const templateFn = tryGetDivPatchTemplateFn({ data, reqLog, templateId });

    const result = tryRenderDivPatchTemplate({
        templateId,
        templateData,
        templateFn,
        mmRequest,
        requestState,
        reqLog,
    });

    return { result, requestState, data, templateId };
};
