import promiseFinally from 'promise.prototype.finally';
import { getKeys } from '../utils/common';

promiseFinally.shim();

/**
 * @link https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/entries#Polyfill
 */
if (!Object.entries) {
    Object.entries = <T>(obj: { [s in string]: T }): Array<[string, T]> => {
        const ownProps = getKeys(obj);
        let i = ownProps.length;
        const resArray: Array<[string, T]> = new Array(i);

        while (i--) {
            resArray[i] = [ownProps[i], obj[ownProps[i]]];
        }

        return resArray;
    };
}

/**
 * @see Object.entries
 */
if (!Object.values) {
    Object.values = <T>(obj: { [s in string]: T }): T[] => {
        const ownProps = getKeys(obj);
        let i = ownProps.length;
        const resArray: T[] = new Array(i);

        while (i--) {
            resArray[i] = obj[ownProps[i]];
        }

        return resArray;
    };
}

/**
 * @link https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/includes#Polyfill
 * @see https://tc39.github.io/ecma262/#sec-array.prototype.includes
 */
if (!Array.prototype.includes) {
    Object.defineProperty(Array.prototype, 'includes', {
        value(searchElement: any, fromIndex: number) {
            if (this == null) {
                throw new TypeError('"this" is null or not defined');
            }

            // 1. Let O be ? ToObject(this value).
            const o = Object(this);

            // 2. Let len be ? ToLength(? Get(O, "length")).
            const len = o.length >>> 0; // tslint:disable-line:no-bitwise

            // 3. If len is 0, return false.
            if (len === 0) {
                return false;
            }

            // 4. Let n be ? ToInteger(fromIndex).
            //    (If fromIndex is undefined, this step produces the value 0.)
            const n = fromIndex | 0; // tslint:disable-line:no-bitwise

            // 5. If n ≥ 0, then
            //  a. Let k be n.
            // 6. Else n < 0,
            //  a. Let k be len + n.
            //  b. If k < 0, let k be 0.
            let k = Math.max(n >= 0 ? n : len - Math.abs(n), 0);

            function sameValueZero(x: any, y: any) {
                return x === y || (typeof x === 'number' && typeof y === 'number' && isNaN(x) && isNaN(y));
            }

            // 7. Repeat, while k < len
            while (k < len) {
                // a. Let elementK be the result of ? Get(O, ! ToString(k)).
                // b. If SameValueZero(searchElement, elementK) is true, return true.
                if (sameValueZero(o[k], searchElement)) {
                    return true;
                }
                // c. Increase k by 1.
                k++;
            }

            // 8. Return false
            return false;
        },
    });
}

// https://developer.mozilla.org/ru/docs/Web/JavaScript/Reference/Global_Objects/String/endsWith#Polyfill
if (!String.prototype.endsWith) {
    Object.defineProperty(String.prototype, 'endsWith', {
        value(searchString: string, position: number) {
            const subjectString = this.toString();
            if (position === undefined || position > subjectString.length) {
                position = subjectString.length;
            }
            position -= searchString.length;
            const lastIndex = subjectString.indexOf(searchString, position);
            return lastIndex !== -1 && lastIndex === position;
        },
    });
}
