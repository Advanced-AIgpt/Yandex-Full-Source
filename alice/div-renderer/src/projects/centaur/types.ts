import type {
    IDualScreenWithOrientationTemplateProps,
} from './components/DualScreen/DualScreenWithOrientation/DualScreenWithOrientationTemplate';
import { IMusicLoaderProps } from './templates/Music/MusicLoader/MusicLoaderTemplate';
import { IDualScreenProps } from './components/DualScreen/DualScreenTemplate';
import { IMusicCardTemplateProps } from './templates/MainScreenMusicTab/templates/MusicLine/MusicCardTemplate';

export interface CentaurTemplateTypes {
    music_playing: {};
    dual_screen_with_orientation: IDualScreenWithOrientationTemplateProps;
    dual_screen_with_orientation_inverse: IDualScreenWithOrientationTemplateProps;
    dual_screen: IDualScreenProps;
    dual_screen_inverse: IDualScreenProps;
    music_loader: IMusicLoaderProps;
    music_card: IMusicCardTemplateProps;
    music_card_with_subtitle: IMusicCardTemplateProps;
    default_loader_content: {};
}
