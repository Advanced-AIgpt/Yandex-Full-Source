import { createShowViewClientAction } from '../../../actions/client';
import { centaurTemplatesClass } from '../../../index';
import { IRequestState } from '../../../../../common/types/common';
import { getImageCoverByLink } from '../../FullMusic/GetImageCoverByLink';
import { TopLevelCard } from '../../../helpers/helpers';
import { ExpFlags } from '../../../expFlags';
import { directivesAction } from '../../../../../common/actions';

interface Props {
    Id?: string | null,
    subtitle?: string | null,
    ImageUrl?: string | null,
    title: string,
}

export function musicLoaderElement({
    Id,
    subtitle,
    ImageUrl,
    title,
}: Props, requestState: IRequestState) {
    const experimentMusicLoader = requestState.hasExperiment(ExpFlags.musicPerformanceOpen);
    if (experimentMusicLoader) {
        requestState.globalTemplates.add('music_loader');
    } else {
        requestState.globalTemplates.add('default_loader_content');
    }

    const imageUrl = ImageUrl ? getImageCoverByLink(ImageUrl, '700x700') : '';

    return {
        log_id: 'main_screen.music.click_loader.' + Id,
        url: directivesAction(createShowViewClientAction(
            TopLevelCard({
                log_id: 'player_loader',
                states: [
                    {
                        state_id: 0,
                        div: experimentMusicLoader ? centaurTemplatesClass.use('music_loader', {
                            header: subtitle || ' ',
                            title,
                            coverURI: imageUrl,
                            playlist_name: ' ',
                        }, requestState) : centaurTemplatesClass.use<'', 'default_loader_content'>('default_loader_content', {}, requestState),
                    },
                ],
            }, requestState),
            true,
        )),
    };
}
