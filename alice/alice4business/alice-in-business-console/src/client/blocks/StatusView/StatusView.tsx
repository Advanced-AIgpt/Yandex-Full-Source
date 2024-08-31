import React, { FC } from 'react';
import block from 'bem-cn';

import './StatusView.scss';
import { IDevice, Status, statusLabels } from '../../model/device';
import Plus from '../Plus/Plus';
import { useRoutes } from '../../context/routes';
import { Icon } from 'lego-on-react';

interface Props extends Pick<IDevice, 'online' | 'status' | 'isActivatedByCustomer' | 'hasPromo'> {
    pending?: boolean;
    activatePromoHandler?: () => void;
}

const b = block('StatusView');

const StatusView: FC<Props> = ({ online, status, isActivatedByCustomer, hasPromo, activatePromoHandler, pending }) => {
    const routes = useRoutes();

    return (
        <div className={b()}>
            <Icon
                cls={b('online')}
                url={online ? routes.assets('images/online.svg') : routes.assets('images/offline.svg')}
            />
            <span className={b('indicator', { status })} />
            <span className={b('message')}>
                {status === Status.Reset && !pending ? 'Ошибка сброса' : statusLabels[status]}
            </span>
            {status === Status.Active && !isActivatedByCustomer && (
                <Plus
                    active={Boolean(hasPromo)}
                    onClick={hasPromo ? () => {} : activatePromoHandler}
                    title={hasPromo ? 'Подписка активирована' : 'Подключить Яндекс.Плюс'}
                />
            )}
        </div>
    );
};

export default StatusView;
