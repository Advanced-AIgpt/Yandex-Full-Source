import block from 'propmods';
import React from 'react';
import Link from '../Link/Link';
import './Menu.scss';

export interface MenuProps {
    items: MenuItem[];
}

export interface MenuItem {
    path: string;
    exact?: boolean;
    title: string;
    disabled?: boolean;
}

const b = block('Menu');

export default function Menu(props: MenuProps) {
    return (
        <ul {...b(props)}>
            {props.items.map((item) => (
                <li {...b('item')} key={item.path}>
                    <Link disabled={item.disabled} to={item.path} exact={item.exact}>
                        {item.title}
                    </Link>
                </li>
            ))}
        </ul>
    );
}
