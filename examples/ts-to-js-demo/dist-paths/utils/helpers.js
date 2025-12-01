

function first(arr) {
    return arr.length > 0 ? arr[0] : undefined;
}

function last(arr) {
    return arr.length > 0 ? arr[arr.length - 1] : undefined;
}

function unique(arr) {
    const result = [];
    for (const item of arr) {
        if (result.indexOf(item) === -1) {
            result.push(item);
        }
    }
    return result;
}

const capitalize = (str) => {
    if (str.length === 0) return str;
    return str.charAt(0).toUpperCase() + str.slice(1);
};

const truncate = (str, maxLength) => {
    if (str.length <= maxLength) return str;
    return str.slice(0, maxLength - 3) + "...";
};

function clamp(value, min, max) {
    return Math.min(Math.max(value, min), max);
}

function random(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function isString(value) {
    return typeof value === "string";
}

function isNumber(value) {
    return typeof value === "number" && !isNaN(value);
}

async function delay(ms) {
    return new Promise((resolve) => {
        setTimeout(resolve, ms);
    });
}

module.exports = { capitalize, truncate, first, last, unique, clamp, random, isString, isNumber, delay };
