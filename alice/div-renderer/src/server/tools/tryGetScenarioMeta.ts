import { RequestError } from '@yandex-int/apphost-lib';
import { logger } from '../../common/logger';
import { NAlice } from '../../protos';

interface TryGetScenarioMetaParams {
    data: NAlice.NRenderer.TDivRenderData;
    reqLog: typeof logger;
}
export const tryGetScenarioMeta = ({ data, reqLog }: TryGetScenarioMetaParams) => {
    const scenarioData = NAlice.NData.TScenarioData.create(data.ScenarioData ?? data.DivPatchData ?? {});

    const templateId = scenarioData.Data;
    if (!templateId) {
        reqLog.warn({ payload: data }, 'Invalid proto input');
        throw new RequestError('templateId missing in proto');
    }

    const templateData = scenarioData[templateId];
    if (!templateData) {
        throw new Error('Empty template data');
    }

    return { templateId, templateData };
};
