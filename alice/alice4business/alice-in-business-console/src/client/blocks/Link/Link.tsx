import H from 'history';
import React from 'react';
import { NavLink } from 'react-router-dom';

import block from 'propmods';

import './Link.scss';

const b = block('Link');

interface LinkProps {
    icon?: string;
    external?: boolean;
    exact?: boolean;
    to: H.LocationDescriptor;
}

export default function Link(props: LinkProps & React.HTMLProps<HTMLAnchorElement>) {
    const { icon, external, to, children, exact, target, ...rest } = props;
    const cn = b({ hasIcon: icon !== void 0, disabled: rest.disabled, external });
    const style: React.CSSProperties = {};
    if (icon) {
        style.backgroundImage = 'url(' + icon + ')';
    }

    return external ? (
        <a {...cn} href={to} target={target || '_blank'} {...(rest as any)}>
            {icon && <div {...b('icon')} style={style} />}
            {children}
        </a>
    ) : (
        <NavLink {...cn} activeClassName='Link_active' exact={exact} to={to} {...(rest as any)}>
            {icon && <div {...b('icon')} style={style} />}
            {children}
        </NavLink>
    );
}
