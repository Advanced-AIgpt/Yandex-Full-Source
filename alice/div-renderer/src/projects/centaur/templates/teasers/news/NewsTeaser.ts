import { NAlice } from '../../../../../protos';
import { formDate, NewsCardProps } from '../../news/common';
import { DualScreen } from '../../../components/DualScreen/DualScreen';
import { Avatar } from '../../../../../common/helpers/avatar';
import { getImageByTopic } from '../../../helpers/newsHelper';
import getColorSet from '../../../style/colorSet';
import PartSimpleText from '../../../components/DualScreen/partComponents/PartSimpleText';
import { colorMoreThenBlackOpacity48 } from '../../../style/constants';
import EmptyDiv from '../../../components/EmptyDiv';
import { MMRequest } from '../../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../../common/types/common';
import { ExpFlags } from '../../../expFlags';
import PartImage from '../../../components/DualScreen/partComponents/PartImage';
import { TopLevelCard } from '../../../helpers/helpers';

function NewsTeaserDiv({ item, topic, tz }: NewsCardProps, requestState: IRequestState) {
    const theme = getColorSet({
        id: item.Text ?? undefined,
    });

    const imageUrl = (
        item.Image?.Src && Avatar.fromUrl(item.Image.Src)?.setTypeName('zen_scale_1200').toString()
    ) ??
        getImageByTopic(topic);

    const newsTitle = item.Text ?? undefined;
    const newsSource = {
        logo: item.Logo ?? undefined,
        name: item.Agency ?? undefined,
    };
    const newsSubText = Number(item.PubDate) ? formDate(Number(item.PubDate) * 1000 || 0, tz) : undefined;

    if (requestState.hasExperiment(ExpFlags.extendedNewsDesignWithDoubleScreen2)) {
        return DualScreen({
            firstDiv: [new EmptyDiv()],
            secondDiv: PartSimpleText({
                colorSet: theme,
                text: newsTitle,
                source: newsSource,
                subText: newsSubText,
                backgroundImage: imageUrl,
                backgroundColor: colorMoreThenBlackOpacity48,
            }),
            requestState,
            mainColor1: theme.mainColor1,
            mainColor: theme.mainColor,
        });
    }

    return DualScreen({
        firstDiv: PartSimpleText({
            colorSet: theme,
            text: newsTitle,
            source: newsSource,
            subText: newsSubText,
        }),
        requestState,
        secondDiv: PartImage({ imageUrl }),
        inverseOnVertical: true,
        mainColor1: theme.mainColor1,
        mainColor: theme.mainColor,
    });
}

export function NewsTeaser({ NewsItem, Topic, Tz }: NAlice.NData.ITNewsTeaserData, _:MMRequest, requestState: IRequestState) {
    const newsCardProps = { item: NewsItem ?? {}, topic: Topic ?? '', bottomGap: false, tz: Tz ?? '' };
    return TopLevelCard({
        log_id: 'news_scenario',
        states: [
            {
                state_id: 0,
                div: NewsTeaserDiv(newsCardProps, requestState),
            },
        ],
    }, requestState);
}
