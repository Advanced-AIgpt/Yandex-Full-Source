import React, { FC } from 'react';
import block from 'bem-cn';
import { Icon } from 'lego-on-react';
import { useRoutes } from '../../context/routes';
import './Plus.scss';

interface Props {
    active: boolean;
    onClick?: () => void;
    title?: string;
}

const b = block('Plus');

const Plus: FC<Props> = ({ active, onClick, title }) => {
    const routes = useRoutes();

    return (
        <span className={b({ active })} onClick={onClick} title={title}>
            <Icon size='s' style={{ width: 42 }} url={routes.assets(`images/plus${active ? '' : '-inactive'}.svg`)} />
        </span>
    );
};

export default Plus;
