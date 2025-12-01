// Utility functions with TypeScript features

// Generic array utilities
export function first<T>(arr: T[]): T | undefined {
    return arr.length > 0 ? arr[0] : undefined;
}

export function last<T>(arr: T[]): T | undefined {
    return arr.length > 0 ? arr[arr.length - 1] : undefined;
}

export function unique<T>(arr: T[]): T[] {
    const result: T[] = [];
    for (const item of arr) {
        if (result.indexOf(item) === -1) {
            result.push(item);
        }
    }
    return result;
}

// String utilities
export const capitalize = (str: string): string => {
    if (str.length === 0) return str;
    return str.charAt(0).toUpperCase() + str.slice(1);
};

export const truncate = (str: string, maxLength: number): string => {
    if (str.length <= maxLength) return str;
    return str.slice(0, maxLength - 3) + "...";
};

// Number utilities
export function clamp(value: number, min: number, max: number): number {
    return Math.min(Math.max(value, min), max);
}

export function random(min: number, max: number): number {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

// Type guards
export function isString(value: unknown): value is string {
    return typeof value === "string";
}

export function isNumber(value: unknown): value is number {
    return typeof value === "number" && !isNaN(value);
}

// Async utility
export async function delay(ms: number): Promise<void> {
    return new Promise((resolve) => {
        setTimeout(resolve, ms);
    });
}
