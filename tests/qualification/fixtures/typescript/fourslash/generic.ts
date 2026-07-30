// @filename: generic.ts
function identity<T>(value: T): T {
    return value;
}

const value = identity("Nova");
//            ^? const value: string
const checked: string = value;
