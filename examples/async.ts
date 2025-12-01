// Test async/await functionality
async function delay(ms: number): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function fetchUser(id: number): Promise<{id: number, name: string}> {
    await delay(100);
    return { id, name: `User ${id}` };
}

async function main() {
    console.log("Fetching users...");
    
    const user1 = await fetchUser(1);
    console.log("User 1:", user1);
    
    const user2 = await fetchUser(2);
    console.log("User 2:", user2);
    
    // Parallel fetching
    const users = await Promise.all([
        fetchUser(3),
        fetchUser(4),
        fetchUser(5)
    ]);
    
    console.log("Users 3-5:", users);
}

main().catch(console.error);
