import React from 'react';
import { Prompt } from 'react-router-dom';
import { getDisplayName } from '../utils/hocs';
import { assignField, getKeys } from '../utils/common';
import { Form } from '../blocks/Form/Form';
import {
    FormAdditionalProps,
    FormChangeHandler,
    FormContainerOptions,
    FormErrors,
    FormSetStateAndValidate,
    FormState,
    FormStateHandlers,
    InjectedFormProps,
    ValidationSchema,
} from './types';

interface FormContainerProps {
    registerChanges: (hasChanges: boolean) => void;
    hasChanges: boolean;
}

const unsavedAlertMessage = 'У вас есть несохраненные изменения, вы действительно хотите продолжить?';
const allowedAnchorsToJump = ['#brandVerificationWebsite'];

const formContainer = <OuterProps, Values>({
    mapPropsToValues,
    setupValidationSchema,
    handleSubmit,
    showPrompt = true,
}: FormContainerOptions<OuterProps, Values>) => {
    return (Component: React.ComponentType<InjectedFormProps<Values, OuterProps>>) => {
        return class FormContainer extends React.Component<OuterProps & FormContainerProps, FormState<Values>> {
            public displayName = getDisplayName(Component);

            private validationSchema: ValidationSchema<Values>;

            constructor(props: OuterProps & FormContainerProps) {
                super(props);

                this.state = {
                    values: mapPropsToValues(props),
                    errors: {},
                    touched: {},
                    isSubmitting: false,
                    isNonValidationError: false,
                };

                this.validationSchema = setupValidationSchema ? setupValidationSchema(props, this.state) : {};
            }

            public componentDidMount() {
                window.onbeforeunload = this.onBeforeUnload;
            }

            public componentWillUnmount() {
                window.onbeforeunload = null;

                this.props.registerChanges(false);
            }

            public handleSubmit = async () => {
                try {
                    await new Promise((resolve) => this.prepareSubmission(true, resolve));
                    await handleSubmit(this.state.values, this.props, this.stateHandlers);

                    this.props.registerChanges(false);
                    this.refresh();
                } catch (err) {
                    if (this.isValid) {
                        this.setState({
                            isNonValidationError: true,
                        });
                    }

                    return;
                } finally {
                    this.setState({ isSubmitting: false });
                }
            };

            public render() {
                return (
                    <Form onSubmit={this.handleSubmit}>
                        {showPrompt && (
                            <Prompt
                                message={(location) =>
                                    allowedAnchorsToJump.includes(location.hash) || unsavedAlertMessage
                                }
                                when={this.props.hasChanges}
                            />
                        )}
                        <Component {...this.props} {...this.state} {...this.additionalProps} {...this.stateHandlers} />
                    </Form>
                );
            }

            private get isValid() {
                const errors = this.state.errors;
                const nonFalsyErrorsCount = getKeys(errors).reduce((acc, key) => {
                    const error = errors[key];

                    return acc + Number(error instanceof Array ? error.some(Boolean) : Boolean(errors[key]));
                }, 0);

                return nonFalsyErrorsCount === 0;
            }

            private get additionalProps(): FormAdditionalProps {
                return {
                    isValid: this.isValid,
                    isNonValidationError: this.state.isNonValidationError,
                };
            }

            private get stateHandlers(): FormStateHandlers<Values> {
                return {
                    onChange: this.onChange,
                    setErrors: this.setErrors,
                    prepareSubmission: this.prepareSubmission,
                    onCheckboxChange: this.onCheckboxChange,
                    onUploaderChange: this.onUploaderChange,
                    setStateAndValidate: this.setStateAndValidate,
                };
            }

            private onChange: FormChangeHandler<Values> = (field) => (value) => {
                this.props.registerChanges(true);

                const { values, touched } = this.state;

                const newErrors = this.getNewErrors(field, value);
                const newValues = Object.assign({}, values, { [field]: value });
                const newTouched = Object.assign({}, touched, { [field]: true });

                this.setState({
                    values: newValues,
                    errors: newErrors,
                    touched: newTouched,
                });
            };

            private setFieldValue = <K extends keyof Values>(field: K, value: Values[typeof field]) => {
                this.props.registerChanges(true);

                const newValues = assignField(this.state.values, field, value);
                const newTouched = assignField(this.state.touched, field, true);

                this.setState({
                    values: newValues,
                    touched: newTouched,
                    errors: this.getNewErrors(field, value),
                });
            };

            private getNewErrors = <K extends keyof Values>(field: K, value: Values[typeof field]) => {
                const { errors } = this.state;
                const validator = this.validationSchema[field];
                // Если валидатор не установлен, то сбрасываем ошибку для изменяемого поля
                const errorsCausedByFieldChange = validator ? validator(value, this.state) : { [field]: '' };
                return Object.assign({}, errors, errorsCausedByFieldChange);
            };

            private onUploaderChange = (value: string | null, error?: string) => {
                this.props.registerChanges(true);

                const newErrors = Object.assign({}, this.state.errors, { logoId: error });
                const newValues = Object.assign({}, this.state.values, { logoId: value });
                const newTouched = Object.assign({}, this.state.touched, { logoId: true });

                this.setState({
                    values: newValues,
                    errors: newErrors,
                    touched: newTouched,
                });
            };

            private onCheckboxChange = (field: keyof Values) => () => {
                this.setFieldValue(field, !this.state.values[field] as any);
            };
            private setErrors = (errors: FormErrors<Values>, cb?: () => any) => {
                this.setState({ errors }, cb);
            };

            private prepareSubmission = (isSubmitting: boolean, cb?: () => any) => {
                this.setState({ isSubmitting, isNonValidationError: false }, cb);
            };

            private setStateAndValidate: FormSetStateAndValidate<Values> = (
                { errors = this.state.errors, values = this.state.values },
                fieldsToValidate,
            ) => {
                this.props.registerChanges(true);

                const newErrors = fieldsToValidate.reduce<FormErrors<Values>>((prevErrors, field) => {
                    const validator = this.validationSchema[field];
                    const errorsCausedByFieldChange = validator
                        ? validator(values[field], this.state)
                        : { [field]: '' };

                    return Object.assign({}, prevErrors, errorsCausedByFieldChange);
                }, Object.assign({}, errors));

                this.setState({
                    values,
                    errors: newErrors,
                });
            };

            private refresh = () => {
                this.setState({
                    touched: {},
                    errors: {},
                    isNonValidationError: false,
                });
            };

            private onBeforeUnload = () => {
                if (this.props.hasChanges) {
                    return unsavedAlertMessage;
                }
            };
        };
    };
};

export default formContainer;
