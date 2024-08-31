import { NAlice } from '../../../../protos';
import { DualScreen } from '../../components/DualScreen/DualScreen';
import PartSimpleText from '../../components/DualScreen/partComponents/PartSimpleText';
import getColorSet from '../../style/colorSet';
import { colorMoreThenBlackOpacity48 } from '../../style/constants';
import EmptyDiv from '../../components/EmptyDiv';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { ExpFlags } from '../../expFlags';
import PartImage from '../../components/DualScreen/partComponents/PartImage';
import { Avatar } from '../../../../common/helpers/avatar';
import { TopLevelCard } from '../../helpers/helpers';

export default function AfishaTeaser(data: NAlice.NData.ITAfishaTeaserData, mmRequest:MMRequest, requestState: IRequestState) {
    return TopLevelCard({
        log_id: 'afisha_scenario',
        states: [
            {
                state_id: 0,
                div: afishaTeaserDiv(data, mmRequest, requestState),
            },
        ],
    }, requestState);
}

function afishaTeaserDiv(data: NAlice.NData.ITAfishaTeaserData, _:MMRequest, requestState: IRequestState) {
    const teaserSource = {
        logo: 'https://yastatic.net/s3/home/services/block/afisha_new.png',
        name: 'Яндекс Афиша',
    };
    const date = data.Date ?? '';
    const place = data.Place ?? '';
    const text = data.Title ?? undefined;
    const image = (Avatar.fromUrl(data.ImageUrl ?? '')?.setTypeName('s840x840', 'afishanew').toString()) ?? data.ImageUrl ?? '';
    const theme = getColorSet({
        id: text,
    });

    if (requestState.hasExperiment(ExpFlags.extendedNewsDesignWithDoubleScreen2)) {
        return DualScreen({
            requestState,
            firstDiv: [new EmptyDiv()],
            secondDiv: PartSimpleText({
                colorSet: theme,
                text,
                subText: `${date}, ${place}`,
                source: teaserSource,
                backgroundColor: colorMoreThenBlackOpacity48,
                backgroundImage: image,
            }),
            mainColor1: theme.mainColor1,
            mainColor: theme.mainColor,
        });
    }

    return DualScreen({
        requestState,
        firstDiv: PartSimpleText({
            colorSet: theme,
            text,
            subText: `${date}, ${place}`,
            source: teaserSource,
        }),
        secondDiv: PartImage({
            imageUrl: image,
        }),
        mainColor1: theme.mainColor1,
        mainColor: theme.mainColor,
    });
}
