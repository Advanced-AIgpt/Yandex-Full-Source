import {
    ContainerBlock,
    FixedSize,
    GalleryBlock, SolidBackground,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { titleLineHeight } from '../../../components/TitleLine/TitleLine';
import { ItemsList } from './components/ItemsList';
import { colorMoreThenBlack, colorWhite, offsetFromEdgeOfScreen } from '../../../style/constants';
import { suggestButtonHeight } from '../../suggests';
import { NAlice } from '../../../../../protos';
import { getItemListData } from './getData';
import TeaserSettingsWrapper from './TeaserSettingsWrapper';
import { CloseButtonWrapper } from '../../../components/CloseButtonWrapper/CloseButtonWrapper';
import { Layer } from '../../../common/layers';
import { IRequestState } from '../../../../../common/types/common';
import { teaserTriggers } from './components/ItemsListItem';
import { directivesActionWithVariables } from '../../../../../common/actions';
import { createSemanticFrameAction } from '../../../../../common/actions/server';
import { title32m } from '../../../style/Text/Text';
import { TopLevelCard } from '../../../helpers/helpers';
import { ITeaserItemsList } from './types';
import { MMRequest } from '../../../../../common/helpers/MMRequest';


export function ItemsListTeaser(data: NAlice.NData.ITTeaserSettingsWithContentData, requestState: IRequestState) {
    const scenarioData = getItemListData(data);

    for (const item of scenarioData.items) {
        const varName = (item.teaserType + item.teaserId).replace(/-/gi, '');
        if (varName != 'PhotoFrame') {
            requestState.variableTriggers.add(teaserTriggers(varName));
            requestState.variables.add({
                type: 'number',
                name: varName,
                value: item.isChosen ? 1 : 0,
            });
        }
    }
    requestState.variables.add({
        type: 'number',
        name: 'PhotoFrame',
        value: 1,
    });

    return new GalleryBlock({
        paddings: {
            top: titleLineHeight,
            bottom: offsetFromEdgeOfScreen * 2 + suggestButtonHeight,
        },
        height: new FixedSize({ value: 880 }),
        orientation: 'vertical',
        items: compact([
            ...ItemsList(scenarioData?.items ?? []),
            getSaveButton(scenarioData),
        ]),
    });
}

function getSaveButton(scenarioList: ITeaserItemsList) {
    return new ContainerBlock({
        orientation: 'horizontal',
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        paddings: {
            left: 20,
            top: 15,
            right: 20,
            bottom: 20,
        },
        background: [
            new SolidBackground({ color: colorWhite }),
        ],
        border: {
            corner_radius: 28,
        },
        items: [
            new TextBlock({
                ...title32m,
                text_color: colorMoreThenBlack,
                text: 'Сохранить',
                actions: [{
                    log_id: 'save_teaser_settings',
                    url: directivesActionWithVariables(
                        createSemanticFrameAction(
                            {
                                centaur_set_teaser_configuration: {
                                    scenarios_for_teasers_slot: {
                                        teaser_settings_data: {
                                            teaser_settings: getSettings(scenarioList),
                                        },
                                    },
                                },
                            },
                            'CentaurCombinator',
                            'alice.centaur.set_teaser_configuration',
                        ),
                    ),
                }],
            }),
        ],
    });
}

type TeaserSetting = {
    is_chosen: string,
    teaser_config_data: {
        teaser_type: string,
        teaser_id: string
    }
}

function getSettings(scenarioList: ITeaserItemsList): TeaserSetting[] {
    return scenarioList.items.map<TeaserSetting>( scenario =>
        <TeaserSetting> {
            teaser_config_data: {
                teaser_type: scenario.teaserType,
                teaser_id: scenario.teaserId,
            },
            is_chosen: '@{' + (scenario.teaserType + scenario.teaserId).replace(/-/gi, '') + ' == 1.0}',
        },
    );
}

export default function TeaserSettings(data: NAlice.NData.ITTeaserSettingsWithContentData, _:MMRequest, requestState: IRequestState) {
    const content = ItemsListTeaser(data, requestState);

    return TopLevelCard({
        log_id: 'skill_card',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: TeaserSettingsWrapper({
                        children: [content],
                    }),
                    layer: Layer.DIALOG,
                }),
            },
        ],
    }, requestState);
}


