import { MatchParentSize, TextBlock } from 'divcard2';

export function InitCard() {
    return new TextBlock({
        text: 'Init',
        id: 'ss0',
        text_color: '#170d1f',
        alpha: 0.0,
        height: new MatchParentSize(),
        width: new MatchParentSize(),
        visibility_action: {
            log_id: 'inited',
            url: 'div-action://set_state?state_id=1/screens/screen1',
        },
    });
}
