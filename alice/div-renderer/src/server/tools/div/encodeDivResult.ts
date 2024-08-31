import { TemplateCard } from 'divcard2';
import { struct } from 'pb-util/build';
import { IRequestState } from '../../../common/types/common';
import { OneOfKey } from '../../../projects';
import { ExpFlags } from '../../../projects/centaur/expFlags';
import { ITemplateCard } from '../../../projects/centaur/helpers/helpers';
import { NAlice } from '../../../protos';
import { getTemplates, templatesRegistry } from '../../../registries/common';
import { putTimeToEncodeAllProto } from '../../../solomon';
import { trackFn } from '../trackPerformance';

// Soft transition for CENTAUR-1102
const div2CardAsStringBlackList = new Set<string>([
    'CentaurMainScreenGalleryMusicCardData',
    'CentaurMainScreenGalleryVideoCardData',
    'CentaurWidgetCardItemData',
    'CentaurWidgetCardItem',
    'CentaurWidgetGalleryData',
    'DialogovoSkillCardData',
    'CentaurMainScreenMyScreenData',
    'CentaurMainScreenDiscoveryTabData',
    'CentaurMainScreenServicesTabData',
    'NewsTeaserData',
]);

// CENTAUR-1128: black list w/o main screen templates
const div2CardAsStringBlackListReduced = new Set<string>([
    'DialogovoSkillCardData',
    'NewsTeaserData',
]);

export interface DivResultItemToEncode {
    requestState: IRequestState;
    templateId: OneOfKey;
    result: TemplateCard<string> | ITemplateCard;
    data: NAlice.NRenderer.TDivRenderData;
}
export const encodeDivResult = trackFn((dataToEncode: DivResultItemToEncode) => {
    const { requestState, data, templateId, result } = dataToEncode;
    const globalTemplates = Object.assign(
        {},
        getTemplates(requestState.globalTemplates, templatesRegistry) as {},
        requestState.res.globalTemplates.getAll() as {},
    );

    const allowDiv2CardAsStringForTemplate = (requestState.hasExperiment(ExpFlags.scenarioWidgetMechanics) &&
            requestState.hasExperiment(ExpFlags.mainScreenTypedAction))
        ? !div2CardAsStringBlackListReduced.has(templateId)
        : !div2CardAsStringBlackList.has(templateId);

    const isDivCardAsString = requestState.hasExperiment(ExpFlags.div2CardAsString) && allowDiv2CardAsStringForTemplate;

    if (isDivCardAsString) {
        return new NAlice.NRenderer.TRenderResponse({
            CardId: data.CardId,
            StringDiv2Body: JSON.stringify(result),
            GlobalDiv2Templates: {
                [`${templateId}_templates`]: { StringBody: JSON.stringify(globalTemplates) },
            },
            CardName: result.card.log_id,
        });
    } else {
        return new NAlice.NRenderer.TRenderResponse({
            CardId: data.CardId,
            Div2Body: struct.encode(result as {}),
            GlobalDiv2Templates: {
                [`${templateId}_templates`]: {
                    Body: struct.encode(globalTemplates),
                },
            },
            CardName: data.CardName,
        });
    }
}, ({ duration }) => putTimeToEncodeAllProto(duration));
