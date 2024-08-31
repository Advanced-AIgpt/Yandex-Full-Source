import React, { createContext, FC, ReactNode, useContext, useState } from 'react';
import { Modal } from './view';

const DEFAULT_KEY = 'DEFAULT';
const DEFAULT_ZINDEX = 1000;

export interface ShowModalProps {
    component: ((close: () => void) => ReactNode) | ReactNode;
    zIndex?: number;
    key?: string;
    close?: boolean;
    closeOnOutsideClick?: boolean;
}
export interface ModalProps extends Required<ShowModalProps> {
    visible: boolean;
}

interface IModalCtx {
    modals: Record<string, ModalProps>;
    showModal: (option: ShowModalProps) => void;
    closeModal: (key: string) => void;
}

const ModalCtx = createContext<IModalCtx>({} as any);

export const useModalCtx = () => useContext(ModalCtx);

export const ModalWrap: FC = ({ children }) => {
    const [modals, setModals] = useState<Record<string, ModalProps>>({});

    const calcZIndex = () =>
        Object.values(modals).reduce((max, { zIndex }) => Math.max(max, zIndex || DEFAULT_ZINDEX), DEFAULT_ZINDEX);

    const createModal = (options: ModalProps) => {
        setModals((prev) => ({ ...prev, [options.key]: { ...options } }));
    };

    const updateModal = (key: string, options: Partial<ModalProps>) => {
        setModals((prev) => ({ ...prev, [key]: { ...prev[key], ...options } }));
    };

    const showModal = (options: ShowModalProps) => {
        const {
            key = DEFAULT_KEY,
            zIndex = calcZIndex(),
            component,
            close = true,
            closeOnOutsideClick = true,
        } = options;
        if (!modals[key]) {
            createModal({ key, component, zIndex, close, visible: true, closeOnOutsideClick });
        } else {
            updateModal(key, { visible: true, component, close, closeOnOutsideClick });
        }
    };
    const closeModal = (key: string) => updateModal(key, { visible: false, component: null });

    const renderModal = (modal: ModalProps) => {
        const close = () => closeModal(modal.key);
        const component = typeof modal.component === 'function' ? modal.component(close) : modal.component;

        return <Modal {...modal} closeFn={close} component={component} />;
    };

    return (
        <ModalCtx.Provider value={{ modals, showModal, closeModal }}>
            {Object.values(modals).map(renderModal)}
            {children}
        </ModalCtx.Provider>
    );
};
