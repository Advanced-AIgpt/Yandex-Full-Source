import { Div } from 'divcard2';
import { createRequestState } from '../../registries/common';
import { MusicCardTemplate } from './templates/MainScreenMusicTab/templates/MusicLine/MusicCardTemplate';
import { MusicLoaderTemplate } from './templates/Music/MusicLoader/MusicLoaderTemplate';
import { CloseButtonWrapper } from './components/CloseButtonWrapper/CloseButtonWrapper';
import { LoaderDiv } from './components/Loader/Loader';
import { Layer } from './common/layers';
import DualScreenWithOrientationTemplate
    from './components/DualScreen/DualScreenWithOrientation/DualScreenWithOrientationTemplate';
import MusicPlayingTemplate from './templates/Music/MusicPlaying/MusicPlayingTemplate';
import { TemplatesHelperClass } from '../../common/templates/templates';
import type { CentaurTemplateTypes } from './types';
import DualScreenTemplate from './components/DualScreen/DualScreenTemplate';
import { IRequestState } from '../../common/types/common';

function defaultLoader(layer: Layer): [Div, IRequestState] {
    return [CloseButtonWrapper({
        div: LoaderDiv(),
        layer,
    }), createRequestState()];
}

export const centaurTemplatesClass = new TemplatesHelperClass<CentaurTemplateTypes>();

centaurTemplatesClass
    .add('music_playing', MusicPlayingTemplate())
    .add('dual_screen_with_orientation', DualScreenWithOrientationTemplate())
    .add('dual_screen_with_orientation_inverse', DualScreenWithOrientationTemplate(true))
    .add('dual_screen', DualScreenTemplate())
    .add('dual_screen_inverse', DualScreenTemplate(true))
    .add('music_loader', MusicLoaderTemplate())
    .add('music_card', MusicCardTemplate())
    .add('music_card_with_subtitle', MusicCardTemplate(true))
    .add('default_loader_content', defaultLoader(Layer.CONTENT));
