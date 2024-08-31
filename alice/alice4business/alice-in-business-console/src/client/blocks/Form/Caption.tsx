import block from 'propmods';
import React from 'react';

import Link from '../Link/Link';

import './Caption.scss';

interface CaptionProps {
    offsetTop?: boolean;
    offsetBottom?: boolean;
    wide?: boolean;
    url?: string;
}

const b = block('Caption');

const Caption: React.SFC<CaptionProps> = ({ children, url, offsetTop = true, offsetBottom = false, wide = false }) => (
    <span
        {...b({
            'offset-top': offsetTop,
            wide,
            'offset-bottom': offsetBottom,
        })}
    >
        <span {...b('text')}>{children}</span>
        {url && (
            <span {...b('link')}>
                <Link to={url} target='_blank'>
                    ?
                </Link>
            </span>
        )}
    </span>
);

export default Caption;
