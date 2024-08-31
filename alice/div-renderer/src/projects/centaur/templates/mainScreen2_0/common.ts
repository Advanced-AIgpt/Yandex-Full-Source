import { IDivAction } from 'divcard2';
import { EnumLayer } from '../../actions/client';
import { CrutchLoader } from '../../components/Loader/Loader';

interface IPutLoaderToActionOptions {
    action?: string | null,
    layer: EnumLayer,
    actionLogId?: string,
}

export const putLoaderToAction = ({
    action,
    layer,
    actionLogId = 'action_with_loader',
}: IPutLoaderToActionOptions): IDivAction[] | undefined => {
    if (!action) {
        return undefined;
    }
    return [
        CrutchLoader(layer),
        {
            log_id: actionLogId,
            url: action,
        },
    ];
};
