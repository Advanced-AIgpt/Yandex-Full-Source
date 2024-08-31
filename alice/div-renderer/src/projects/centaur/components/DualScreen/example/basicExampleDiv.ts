import { Div } from 'divcard2';
import { createShowViewClientAction, EnumLayer } from '../../../actions/client';
import { TopLevelCard } from '../../../helpers/helpers';

export default function basicExampleDiv(div: Div) {
    return createShowViewClientAction(
        TopLevelCard({
            log_id: 'example',
            states: [
                {
                    state_id: 0,
                    div,
                },
            ],
        }),
        false,
        EnumLayer.dialog,
    );
}
