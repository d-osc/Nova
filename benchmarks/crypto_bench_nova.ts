import { createHash, createHmac, randomBytesHex, randomUUID, pbkdf2Sync } from "crypto";

const iterations = {
    hash: 10000,
    hmac: 5000,
    random: 5000,
    pbkdf2: 100
};

console.log('Nova Crypto Benchmark');
console.log('');

// Test data
const testData = 'The quick brown fox jumps over the lazy dog';
const testKey = 'secret-key-12345';
const testPassword = 'password';
const testSalt = 'salt';

// Benchmark: SHA256 Hash
console.log('SHA256 (' + iterations.hash + ' iterations):');
let start = Date.now();
for (let i = 0; i < iterations.hash; i++) {
    const hash = createHash('sha256', testData);
}
let time = Date.now() - start;
console.log('  Time: ' + time + 'ms');
const hashOps = Math.floor(iterations.hash / (time / 1000));
console.log('  Ops/sec: ' + hashOps);

// Benchmark: MD5 Hash
console.log('');
console.log('MD5 (' + iterations.hash + ' iterations):');
start = Date.now();
for (let i = 0; i < iterations.hash; i++) {
    const hash = createHash('md5', testData);
}
time = Date.now() - start;
console.log('  Time: ' + time + 'ms');
const md5Ops = Math.floor(iterations.hash / (time / 1000));
console.log('  Ops/sec: ' + md5Ops);

// Benchmark: HMAC-SHA256
console.log('');
console.log('HMAC-SHA256 (' + iterations.hmac + ' iterations):');
start = Date.now();
for (let i = 0; i < iterations.hmac; i++) {
    const hmac = createHmac('sha256', testKey, testData);
}
time = Date.now() - start;
console.log('  Time: ' + time + 'ms');
const hmacOps = Math.floor(iterations.hmac / (time / 1000));
console.log('  Ops/sec: ' + hmacOps);

// Benchmark: Random Bytes
console.log('');
console.log('RandomBytesHex(32) (' + iterations.random + ' iterations):');
start = Date.now();
for (let i = 0; i < iterations.random; i++) {
    const bytes = randomBytesHex(32);
}
time = Date.now() - start;
console.log('  Time: ' + time + 'ms');
const randOps = Math.floor(iterations.random / (time / 1000));
console.log('  Ops/sec: ' + randOps);

// Benchmark: Random UUID
console.log('');
console.log('RandomUUID (' + iterations.random + ' iterations):');
start = Date.now();
for (let i = 0; i < iterations.random; i++) {
    const uuid = randomUUID();
}
time = Date.now() - start;
console.log('  Time: ' + time + 'ms');
const uuidOps = Math.floor(iterations.random / (time / 1000));
console.log('  Ops/sec: ' + uuidOps);

// Benchmark: PBKDF2
console.log('');
console.log('PBKDF2 (' + iterations.pbkdf2 + ' iterations, 1000 rounds):');
start = Date.now();
for (let i = 0; i < iterations.pbkdf2; i++) {
    const key = pbkdf2Sync(testPassword, testSalt, 1000, 32, 'sha256');
}
time = Date.now() - start;
console.log('  Time: ' + time + 'ms');
const pbkdfOps = Math.floor(iterations.pbkdf2 / (time / 1000));
console.log('  Ops/sec: ' + pbkdfOps);

console.log('');
console.log('--- Benchmark Complete ---');
