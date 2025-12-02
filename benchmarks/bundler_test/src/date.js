export function formatDate(date) {
    return date.toISOString().split('T')[0];
}

export function parseDate(str) {
    return new Date(str);
}

export function addDays(date, days) {
    const result = new Date(date);
    result.setDate(result.getDate() + days);
    return result;
}

export function diffDays(date1, date2) {
    const diff = Math.abs(date2 - date1);
    return Math.ceil(diff / (1000 * 60 * 60 * 24));
}
