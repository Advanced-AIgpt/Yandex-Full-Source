import React from 'react';
import { useModalCtx } from './service';
import { Confirm, ConfirmProps } from './view';

export const useModals = () => {
    const { showModal } = useModalCtx();

    const showConfirm = (props: ConfirmProps) =>
        showModal({
            component: (close) => (
                <Confirm
                    onReject={() => {
                        if (props.onReject) {
                            props.onReject();
                        }
                        close();
                    }}
                    {...props}
                    onConfirm={() => {
                        props.onConfirm();
                        close();
                    }}
                />
            ),
            close: false,
        });

    return {
        showModal,
        showConfirm,
    };
};
