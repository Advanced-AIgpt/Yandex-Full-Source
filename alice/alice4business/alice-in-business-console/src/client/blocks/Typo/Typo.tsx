import block from 'propmods';
import React, { FC } from 'react';
import './Typo.scss';

function makeBemEntity<T = {}>(blockName: string, elementName?: string) {
    return (props: React.HTMLProps<HTMLDivElement & T>) => {
        const b = elementName ? block(blockName)(elementName, props) : block(blockName)(props);

        return <div {...b} {...props} />;
    };
}

export function H1(props: React.HTMLProps<HTMLHeadingElement>) {
    return <h2 {...block('H1')(props)} {...props} />;
}

export function H2(props: React.HTMLProps<HTMLHeadingElement>) {
    return <h2 {...block('H2')(props)} {...props} />;
}

export function H3(props: React.HTMLProps<HTMLHeadingElement>) {
    return <h2 {...block('H3')(props)} {...props} />;
}

export const HeaderTitle: FC = ({ children }) => <div {...block('header-title')()}>{children}</div>;

interface PageHeadlineProps {
    title: string | React.ReactNode;
    subtitle?: string | React.ReactNode;
}

export function PageHeadline({ title, subtitle }: PageHeadlineProps) {
    const b = block('PageHeadline');

    return (
        <h2 {...b()}>
            <span {...b('title', { alone: !subtitle })}>{title}</span>
            {subtitle && <span {...b('subtitle')}>{subtitle}</span>}
        </h2>
    );
}

export const PageContent = makeBemEntity('PageContent');
export const PageMain = makeBemEntity('PageMain');
export const PageAside = makeBemEntity('PageAside');
export const PageHeadline2 = makeBemEntity('PageHeadline2');
export const PageMain2 = makeBemEntity('PageMain2');
export const PageAside2 = makeBemEntity('PageAside2');
export const Page = Object.assign(makeBemEntity('Page'), {
    Head: makeBemEntity('Page', 'head'),
    HeadContent: makeBemEntity('Page', 'head-content'),
    Content: makeBemEntity('Page', 'content'),
    Page: makeBemEntity('Page', 'page'),
});
