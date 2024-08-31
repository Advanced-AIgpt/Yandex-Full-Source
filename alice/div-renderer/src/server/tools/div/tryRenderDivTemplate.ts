import { TemplateCard } from 'divcard2';
import { attempt } from 'lodash';
import { MMRequest } from '../../../common/helpers/MMRequest';
import { logger } from '../../../common/logger';
import { IRequestState } from '../../../common/types/common';
import { OneOfKey, TemplateData, Templates } from '../../../projects';
import { ExpFlags } from '../../../projects/centaur/expFlags';
import { ITemplateCard } from '../../../projects/centaur/helpers/helpers';
import { incTemplateFails, incTemplateRequest, putTemplateRenderTime } from '../../../solomon';
import { trackFn } from '../trackPerformance';
import { config } from '../../config';
import { HumanError } from '../../../projects/centaur/components/HumanError/HumanError';

interface TryRenderDivTemplateParams {
    reqLog: typeof logger;
    templateId: OneOfKey;
    templateFn: NonNullable<Templates[OneOfKey]>;
    templateData: TemplateData;
    mmRequest: MMRequest;
    requestState: IRequestState;
}
export const tryRenderDivTemplate = trackFn(
    ({
        templateId,
        templateFn,
        templateData,
        mmRequest,
        requestState,
        reqLog,
    }: TryRenderDivTemplateParams) => {
        reqLog.info(`Start rendering template "${templateId}"`);

        const result: TemplateCard<string> | ITemplateCard | Error = attempt(
            templateFn,
            templateData,
            mmRequest,
            requestState,
        );

        if (result instanceof Error) {
            incTemplateFails(templateId);

            reqLog.error(result, `Failed to render ${templateId}`);

            if (requestState.hasExperiment(ExpFlags.showError) === false) {
                throw result;
            }

            return HumanError({
                error: result,
                data: templateData,
            }, mmRequest, requestState);
        }

        incTemplateRequest(templateId);

        setTimeout(() => reqLog.debug({ result }, `Rendering template "${templateId}" finished successfully`));

        reqLog.info(`Rendering template "${templateId}" finished successfully`);

        return result;
    },
    ({ duration, args: [{ templateId, reqLog }] }) => {
        putTemplateRenderTime(templateId, duration);

        setTimeout(() => reqLog.debug({ duration, templateId }, `Rendering template "${templateId}" finished successfully`));

        if (duration > config.templateSlowRequest) {
            reqLog.warn({ duration, templateId }, `Rendering template "${templateId}" took long`);
        }
    },
);
