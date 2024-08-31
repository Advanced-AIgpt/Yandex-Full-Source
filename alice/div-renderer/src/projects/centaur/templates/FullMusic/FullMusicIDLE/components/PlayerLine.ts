import { ContainerBlock } from 'divcard2';
import { AlbumCover } from './AlbumCover';
import { PlayerControls } from '../../components/PlayerControls/PlayerControls';

interface Props {
    coverUri: string,
    trackId: string,
}

export function PlayerLine({
    coverUri,
    trackId,
}: Props) {
    return new ContainerBlock({
        orientation: 'horizontal',
        margins: {
            left: 14,
        },
        items: [
            AlbumCover({
                coverUri,
                trackId,
            }),
            PlayerControls(),
        ],
    });
}
