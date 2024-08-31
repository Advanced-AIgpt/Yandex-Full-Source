import { createContext } from 'react';

interface FieldContextParams {
    nonValidationError: React.ReactNode;
    setNonValidationError: (error: React.ReactNode) => void;
}

const FormFieldContext = createContext<FieldContextParams>({
    nonValidationError: '',
    setNonValidationError: () => undefined,
});

export default FormFieldContext;
