import block from 'propmods';
import React, { FC } from 'react';
import './SettingsPage.scss';
import { CheckBox } from 'lego-on-react';

const b = block('SettingsPage');

const SettingsPage: FC = () => {
    return (
        <div {...b()}>
            <CheckBox theme='normal' size='s' tone='default' view='default' checked={true}>
                Активировать промокод
            </CheckBox>
        </div>
    );
};

export default SettingsPage;
