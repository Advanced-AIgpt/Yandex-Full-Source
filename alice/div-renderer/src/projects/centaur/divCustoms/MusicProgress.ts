import { DivCustomBlock, MatchParentSize } from 'divcard2';

export default function MusicProgress() {
    return new DivCustomBlock({
        custom_type: 'centaur_music_progress',
        width: new MatchParentSize(),
        height: new MatchParentSize({ weight: 1 }),
        extensions: [
            {
                id: 'music-player-progressbar',
            },
        ],
    });
}
