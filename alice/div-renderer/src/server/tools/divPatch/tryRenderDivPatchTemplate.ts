import { attempt } from 'lodash';
import { MMRequest } from '../../../common/helpers/MMRequest';
import { logger } from '../../../common/logger';
import { IRequestState } from '../../../common/types/common';
import { IDivPatchElement, IDivPatchTemplates, OneOfKey, TemplateData } from '../../../projects';
import { incTemplateFails, incTemplateRequest, putTemplateRenderTime } from '../../../solomon';
import { trackFn } from '../trackPerformance';
import { config } from '../../config';

interface TryRenderDivPatchTemplateParams {
    reqLog: typeof logger;
    templateId: OneOfKey;
    templateFn: NonNullable<IDivPatchTemplates[OneOfKey]>;
    templateData: TemplateData;
    mmRequest: MMRequest;
    requestState: IRequestState;
}
export const tryRenderDivPatchTemplate = trackFn(
    ({
        templateId,
        templateFn,
        templateData,
        mmRequest,
        requestState,
        reqLog,
    }: TryRenderDivPatchTemplateParams) => {
        reqLog.info(`Start rendering template "${templateId}"`);

        const result: IDivPatchElement[] | Error = attempt(
            templateFn,
            templateData,
            mmRequest,
            requestState,
        );

        if (result instanceof Error) {
            incTemplateFails(templateId);

            reqLog.error(result, `Failed to render div patch ${templateId}`);

            throw result;
        }

        incTemplateRequest(templateId);

        setTimeout(() => reqLog.debug({ result }, `Rendering div patch template "${templateId}" finished successfully`));

        reqLog.info(`Rendering div patch template "${templateId}" finished successfully`);

        return result;
    },
    ({ duration, args: [{ templateId, reqLog }] }) => {
        putTemplateRenderTime(templateId, duration);

        setTimeout(() => reqLog.debug({ duration, templateId }, `Rendering div patch template "${templateId}" finished successfully`));

        if (duration > config.templateSlowRequest) {
            reqLog.warn({ duration, templateId }, `Rendering div patch template "${templateId}" took long`);
        }
    },
);
