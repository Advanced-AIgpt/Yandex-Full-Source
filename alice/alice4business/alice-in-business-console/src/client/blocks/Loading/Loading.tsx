import block from 'propmods';
import React from 'react';

import { Spin, SpinSize } from 'lego-on-react';

import './Loading.scss';

const b = block('Loading');

interface Props {
    size?: SpinSize;
    centered?: boolean;
    className?: string;
    grow?: boolean;
    margin?: string;
}

const Loading: React.SFC<Props> = ({ size = 'l', centered, grow, margin }) => (
    <div {...b({ centered, grow })} style={{ margin }}>
        <div {...b('spinner')}>
            <Spin size={size} progress />
        </div>
    </div>
);

export default Loading;
