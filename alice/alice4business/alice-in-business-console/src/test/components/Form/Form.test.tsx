import React from 'react';
import { Field } from '../../../client/blocks/Form/Form';

test('Field should render', () => {
    const field = enzyme.shallow(<Field />);

    expect(field.exists('.Form__fieldBody')).toBeTruthy();
});
