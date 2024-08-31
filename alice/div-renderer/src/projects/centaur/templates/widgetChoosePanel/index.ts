import { TemplateCard, Templates } from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { getCard, ICardInfo } from '../mainScreen2_0/Cards';
import WidgetsListWrapper from './WidgetsListWrapper';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';
import { Layer } from '../../common/layers';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { ExpFlags, hasExperiment } from '../../expFlags';
import { IRequestState } from '../../../../common/types/common';

export default function WidgetChoosePanel(
    { Widgets: Widgets, WidgetCards: WidgetCards }: NAlice.NData.ITCentaurWidgetGalleryData, mmRequest: MMRequest, requestState: IRequestState) {
    const widgetsDiv: ICardInfo[] = hasExperiment(mmRequest, ExpFlags.scenarioWidgetMechanics) ?
        compact(WidgetCards?.map(col => getCard(col, requestState, { isChoice: true }))) :
        compact(Widgets?.map(col => getCard(col, requestState, { isChoice: true })));
    return new TemplateCard(new Templates({}), {
        log_id: 'widgetslist',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: WidgetsListWrapper({ widgets: widgetsDiv }),
                    layer: Layer.CONTENT,
                }),
            },
        ],
    });
}
