import { TemplateCard, Templates } from 'divcard2';
import { directivesAction } from '../../../../common/actions';
import { createShowViewClientAction } from '../../actions/client';
import { FullMusicOffDiv } from './FullMusicOff/FullMusicOff';
import { IRequestState } from '../../../../common/types/common';

interface Props {
    Id?: string | null,
    subtitle?: string | null,
    ImageUrl?: string | null,
    title: string,
    requestState: IRequestState;
}

export function musicLoaderElement({
    Id,
    subtitle,
    ImageUrl,
    title,
    requestState,
}: Props) {
    return {
        log_id: 'main_screen.music.click_loader.' + Id,
        url: directivesAction(createShowViewClientAction(
            new TemplateCard(new Templates({}), {
                log_id: 'player_loader',
                states: [
                    {
                        state_id: 0,
                        div: FullMusicOffDiv({
                            header: ' ',
                            artist: subtitle ?? '',
                            imageUri: ImageUrl ?? '',
                            trackId: '',
                            audio_source_id: '',
                            title,
                            direction: 'bottom',
                            isLiked: false,
                            isDisliked: false,
                            requestState,
                        }),
                    },
                ],
            }),
            false,
        )),
    };
}
