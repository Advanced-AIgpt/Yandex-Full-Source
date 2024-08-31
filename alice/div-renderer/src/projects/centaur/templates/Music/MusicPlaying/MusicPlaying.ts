import { IRequestState } from '../../../../../common/types/common';
import { centaurTemplatesClass } from '../../../index';

export default function MusicPlaying(requestState: IRequestState) {
    return centaurTemplatesClass.use('music_playing', {}, requestState);
}
