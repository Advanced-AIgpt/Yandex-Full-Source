let POPUPS_CACHE: Map<string, Window> | null = null;

const findLeftWindowBoundry = () => {
    // In Internet Explorer window.screenLeft is the window's left boundry
    if (window.screenLeft) {
        return window.screenLeft;
    }

    // In Firefox window.screenX is the window's left boundry
    if (window.screenX) {
        return window.screenX;
    }

    return 0;
};

const findTopWindowBoundry = () => {
    // In Internet Explorer window.screenLeft is the window's left boundry
    if (window.screenTop) {
        return window.screenTop;
    }

    // In Firefox window.screenY is the window's left boundry
    if (window.screenY) {
        return window.screenY;
    }

    return 0;
};

export const openPopup = (name: string, url: string): Window | null => {
    if (!POPUPS_CACHE) {
        POPUPS_CACHE = new Map();
    }

    let popupRef = null;
    if (POPUPS_CACHE.has(name)) {
        popupRef = POPUPS_CACHE.get(name);
    }

    if (popupRef && !popupRef.closed) {
        popupRef.focus();
    } else {
        const width = 800;
        const height = 700;
        const left =
            window.outerWidth / 2 - width / 2 + findLeftWindowBoundry();
        const top =
            window.outerHeight / 2 - height / 2 + findTopWindowBoundry();

        const params = { width, height, left, top };
        const paramsStr = Object.entries(params)
            .map(([k, v]) => `${k}=${v}`)
            .join(',');

        popupRef = window.open(url, name, paramsStr);
    }

    return popupRef;
};
