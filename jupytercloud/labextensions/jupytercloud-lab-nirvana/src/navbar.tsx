import React from 'react';

import { Navbar, Nav } from 'react-bootstrap';

const NIRVANA_DOCS_URL =
    'https://wiki.yandex-team.ru/jupyter/docs/nirvanaexport';

interface INirvanaExportNavbar {
    backToNotebook: () => void;
}

export const NirvanaExportNavbar = (props: INirvanaExportNavbar) => {
    return (
        <Navbar>
            <Navbar.Brand>Export to Nirvana (β)</Navbar.Brand>
            <Nav>
                <Nav.Link onClick={props.backToNotebook}>
                    Back to notebook
                </Nav.Link>
                <Nav.Link href={NIRVANA_DOCS_URL} target="_blank">
                    Nirvana export docs
                </Nav.Link>
            </Nav>
        </Navbar>
    );
};
