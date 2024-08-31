import block from 'propmods';
import React from 'react';

import './Footer.scss';
import { Copyright } from 'lego-on-react';

const b = block('Footer');

export default function Footer() {
    return (
        <footer {...b()}>
            <div {...b('content')}>
                <div {...b('links')} />
                <Copyright start={2019} />
            </div>
        </footer>
    );
}
