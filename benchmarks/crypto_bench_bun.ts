import crypto from 'crypto';

const iterations = {
    hash: 10000,
    hmac: 5000,
    random: 5000,
    pbkdf2: 100
};

console.log('Bun Crypto Benchmark\n');

// Test data
const testData = 'The quick brown fox jumps over the lazy dog';
const testKey = 'secret-key-12345';
const testPassword = 'password';
const testSalt = 'salt';

// Benchmark: SHA256 Hash
console.log(`SHA256 (${iterations.hash} iterations):`);
let start = Date.now();
for (let i = 0; i < iterations.hash; i++) {
    const hash = crypto.createHash('sha256').update(testData).digest('hex');
}
let time = Date.now() - start;
console.log(`  Time: ${time}ms`);
console.log(`  Ops/sec: ${Math.round(iterations.hash / (time / 1000))}`);

// Benchmark: MD5 Hash
console.log(`\nMD5 (${iterations.hash} iterations):`);
start = Date.now();
for (let i = 0; i < iterations.hash; i++) {
    const hash = crypto.createHash('md5').update(testData).digest('hex');
}
time = Date.now() - start;
console.log(`  Time: ${time}ms`);
console.log(`  Ops/sec: ${Math.round(iterations.hash / (time / 1000))}`);

// Benchmark: HMAC-SHA256
console.log(`\nHMAC-SHA256 (${iterations.hmac} iterations):`);
start = Date.now();
for (let i = 0; i < iterations.hmac; i++) {
    const hmac = crypto.createHmac('sha256', testKey).update(testData).digest('hex');
}
time = Date.now() - start;
console.log(`  Time: ${time}ms`);
console.log(`  Ops/sec: ${Math.round(iterations.hmac / (time / 1000))}`);

// Benchmark: Random Bytes
console.log(`\nRandomBytes(32) (${iterations.random} iterations):`);
start = Date.now();
for (let i = 0; i < iterations.random; i++) {
    const bytes = crypto.randomBytes(32);
}
time = Date.now() - start;
console.log(`  Time: ${time}ms`);
console.log(`  Ops/sec: ${Math.round(iterations.random / (time / 1000))}`);

// Benchmark: Random UUID
console.log(`\nRandomUUID (${iterations.random} iterations):`);
start = Date.now();
for (let i = 0; i < iterations.random; i++) {
    const uuid = crypto.randomUUID();
}
time = Date.now() - start;
console.log(`  Time: ${time}ms`);
console.log(`  Ops/sec: ${Math.round(iterations.random / (time / 1000))}`);

// Benchmark: PBKDF2
console.log(`\nPBKDF2 (${iterations.pbkdf2} iterations, 1000 rounds):`);
start = Date.now();
for (let i = 0; i < iterations.pbkdf2; i++) {
    const key = crypto.pbkdf2Sync(testPassword, testSalt, 1000, 32, 'sha256');
}
time = Date.now() - start;
console.log(`  Time: ${time}ms`);
console.log(`  Ops/sec: ${Math.round(iterations.pbkdf2 / (time / 1000))}`);

console.log('\n--- Benchmark Complete ---');
