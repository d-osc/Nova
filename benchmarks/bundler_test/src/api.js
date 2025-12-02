export async function fetchData(url) {
    return { url, data: 'mock data', timestamp: Date.now() };
}

export async function postData(url, data) {
    return { url, data, status: 'success', timestamp: Date.now() };
}

export async function deleteData(url) {
    return { url, status: 'deleted', timestamp: Date.now() };
}
