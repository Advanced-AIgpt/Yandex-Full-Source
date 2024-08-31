import { ImageBlock, MatchParentSize, TemplateCard, Templates } from 'divcard2';
import { NAlice } from '../../../../protos';

export const renderMainScreenVideo = ({ Action, ImageUrl, Id }: NAlice.NData.ITCentaurMainScreenGalleryVideoCardData) =>
    new TemplateCard(new Templates({}), {
        log_id: 'main_screen.video',
        states: [
            {
                state_id: 0,
                div: new ImageBlock({
                    width: new MatchParentSize(),
                    height: new MatchParentSize(),
                    image_url: ImageUrl ?? '',
                    action: {
                        log_id: 'main_screen.video.click.' + Id,
                        url: Action ?? '',
                    },
                    preload_required: 1,
                    border: {
                        corner_radius: 28,
                    },
                }),
            },
        ],
    });
