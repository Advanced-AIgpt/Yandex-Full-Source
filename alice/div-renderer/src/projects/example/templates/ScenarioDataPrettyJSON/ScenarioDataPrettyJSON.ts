import { TemplateCard, Templates, TextBlock, WrapContentSize } from 'divcard2';
import { CloseButtonWrapper } from '../../../centaur/components/CloseButtonWrapper/CloseButtonWrapper';

interface PrettyJSONProps {
    data: unknown
}
export const PrettyJSON = ({ data }: PrettyJSONProps) => {
    return new TextBlock({
        text: JSON.stringify(data, null, 4),
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        text_alignment_horizontal: 'left',
        text_color: '#ffffff',
        font_size: 30,
        margins: {
            left: 36,
            top: 36,
            bottom: 36,
            right: 36,
        },
    });
};

export default function ScenarioDataPrettyJSON(data: unknown) {
    return new TemplateCard(new Templates({}), {
        log_id: 'scenario_data_pretty_json_card',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: PrettyJSON({ data }),
                }),
            },
        ],
    });
}
