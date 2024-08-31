import { NAlice } from '../../../../protos';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { TopLevelCard } from '../../helpers/helpers';
import { DualScreen } from '../../components/DualScreen/DualScreen';
import getColorSet, { IColorSet } from '../../style/colorSet';
import PartImage from '../../components/DualScreen/partComponents/PartImage';
import { Avatar } from '../../../../common/helpers/avatar';
import PartBigText from '../../components/DualScreen/partComponents/PartBigText';
import { SolidBackground } from 'divcard2';
import { ExpFlags } from '../../expFlags';
import SearchFactOld from './SearchFactOld';
import TopLayout from '../../components/TopLayout';
import { Layer } from '../../common/layers';
import { ISuggest } from '../../components/Suggests/types';
import { compact } from 'lodash';
import Suggests from '../../components/Suggests/Suggests';
import { colorBlackOpacity0 } from '../../style/constants';
type ITSearchFactData = NAlice.NData.ITSearchFactData;

interface ISearchFactDataProps {
    image: string;
    fact: string;
    source?: string;
    suggests: ISuggest[];
}

function getSuggestsByMMRequest(mmRequest: MMRequest, colorSet: IColorSet): ISuggest[] {
    return compact(
        [
            ...(mmRequest.ScenarioResponseBody.Layout?.Suggests?.map(el => {
                return el.Title && el.ActionId && {
                    text: el.Title,
                    colorSet,
                    actions: [
                        {
                            log_id: 'suggest_action',
                            url: el.ActionId,
                        },
                    ],
                };
            }) || []),
        ],
    );
}

function dataAdapter(data: ITSearchFactData, mmRequest: MMRequest, colorSet: IColorSet): ISearchFactDataProps {
    let image = data.Image || '';
    const avatar = Avatar.fromUrl(image);

    if (avatar && avatar.namespace === 'entity_search') {
        image = avatar.setTypeName('S600xU').toString();
    }
    return {
        image,
        source: data.Hostname || undefined,
        fact: data.Text || ' ',
        suggests: getSuggestsByMMRequest(mmRequest, colorSet),
    };
}

export default function SearchFact(
    searchFactData: NAlice.NData.ITSearchFactData,
    mmRequest: MMRequest,
    requestState: IRequestState,
) {
    if (!requestState.hasExperiment(ExpFlags.newFact)) {
        return SearchFactOld(searchFactData, mmRequest);
    }

    const colorSet = getColorSet({
        id: searchFactData.Text || '',
    });

    const {
        image,
        fact,
        source,
        suggests,
    } = dataAdapter(searchFactData, mmRequest, colorSet);

    const suggestsBlock = Suggests({ suggests });

    return TopLevelCard({
        log_id: 'fact_object',
        states: [
            {
                state_id: 0,
                div: TopLayout({
                    closeButton: {
                        layer: Layer.DIALOG,
                        backgroundColor: colorBlackOpacity0,
                    },
                    bottomLine: suggestsBlock ? [
                        suggestsBlock,
                    ] : undefined,
                    content: DualScreen({
                        firstDiv: PartBigText({
                            text: fact,
                            subText: source && `Найдено на ${source}`,
                            colorSet,
                        }),
                        requestState,
                        secondDiv: PartImage({
                            size: 424,
                            imageUrl: image,
                            imageOptions: {
                                border: {
                                    corner_radius: 24,
                                },
                                background: [
                                    new SolidBackground({ color: colorSet.textColorOpacity10 }),
                                ],
                            },
                        }),
                        mainColor1: colorSet.mainColor1,
                        mainColor: colorSet.mainColor,
                    }),
                }),
            },
        ],
    }, requestState);
}
