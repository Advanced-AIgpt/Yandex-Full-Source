import React from 'react';
import block from 'propmods';

import './VerificationError.scss';

const b = block('VerificationError');

interface Props {
    className?: string;
    text?: string;
}

const VerificationError: React.SFC<Props> = ({ className, text = 'Не проверено' }) => {
    return (
        <div
            {...{
                className: `${b().className} ${className}`,
            }}
        >
            <div {...b('sign')} />
            <div {...b('text')}>{text}</div>
        </div>
    );
};

export default VerificationError;
