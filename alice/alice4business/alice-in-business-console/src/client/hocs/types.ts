export type FormErrors<Values> = { [K in keyof Values]?: Values[K] extends any[] ? string[] : string };

export type Validator<T, Values> = (value: T, state: FormState<Values>) => Partial<FormErrors<Values>>;

export type ValidationSchema<Values> = { [K in keyof Values]?: Validator<Values[K], Values> };

export interface FormContainerOptions<OuterProps, Values> {
    showPrompt?: boolean;
    mapPropsToValues: (props: Readonly<OuterProps>) => Values;
    handleSubmit: (
        values: Values,
        props: Readonly<OuterProps>,
        stateHanglers: FormStateHandlers<Values>,
    ) => Promise<void>;
    setupValidationSchema?: (props: Readonly<OuterProps>, state: FormState<Values>) => ValidationSchema<Values>;
}

export interface FormState<Values> {
    values: Values;
    errors: FormErrors<Values>;
    touched: Partial<Record<keyof Values, boolean>>;
    isSubmitting: boolean;
    isNonValidationError: boolean;
}

export type FormChangeHandler<Values> = <K extends keyof Values>(name: K) => (value: Values[typeof name]) => void;

export type FormSetStateAndValidate<Values> = (
    state: Partial<FormState<Values>>,
    fieldsToValidate: Array<keyof Values>,
) => void;

export interface FormStateHandlers<Values> {
    onChange: FormChangeHandler<Values>;
    onCheckboxChange: (field: keyof Values) => () => void;
    setErrors: (errors: FormErrors<Values>, cb?: () => any) => void;
    onUploaderChange: (value: string | null, error?: string) => void;
    prepareSubmission: (isSubmitting: boolean, cb?: () => any) => void;
    setStateAndValidate: FormSetStateAndValidate<Values>;
}

export interface FormAdditionalProps {
    isValid: boolean;
    isNonValidationError: boolean;
}

export type InjectedFormProps<Values, OuterProps> = OuterProps &
    FormAdditionalProps &
    FormState<Values> &
    FormStateHandlers<Values>;
