// Bun Path Module Benchmark
import * as path from 'path';

const iterations = 10000;

// Test paths
const testPaths = [
  '/home/user/documents/file.txt',
  'C:\\Users\\Admin\\Desktop\\project\\src\\index.js',
  '../relative/path/to/file.ts',
  './current/directory/file.json',
  '/var/www/html/index.html'
];

console.log('Bun Path Benchmark');
console.log('==================\n');

// Benchmark dirname
let start = Date.now();
for (let i = 0; i < iterations; i++) {
  for (const p of testPaths) {
    path.dirname(p);
  }
}
let end = Date.now();
const dirnameTime = end - start;
console.log(`dirname: ${dirnameTime}ms (${iterations * testPaths.length} ops, ${((iterations * testPaths.length) / dirnameTime).toFixed(0)} ops/ms)`);

// Benchmark basename
start = Date.now();
for (let i = 0; i < iterations; i++) {
  for (const p of testPaths) {
    path.basename(p);
  }
}
end = Date.now();
const basenameTime = end - start;
console.log(`basename: ${basenameTime}ms (${iterations * testPaths.length} ops, ${((iterations * testPaths.length) / basenameTime).toFixed(0)} ops/ms)`);

// Benchmark extname
start = Date.now();
for (let i = 0; i < iterations; i++) {
  for (const p of testPaths) {
    path.extname(p);
  }
}
end = Date.now();
const extnameTime = end - start;
console.log(`extname: ${extnameTime}ms (${iterations * testPaths.length} ops, ${((iterations * testPaths.length) / extnameTime).toFixed(0)} ops/ms)`);

// Benchmark normalize
start = Date.now();
for (let i = 0; i < iterations; i++) {
  for (const p of testPaths) {
    path.normalize(p);
  }
}
end = Date.now();
const normalizeTime = end - start;
console.log(`normalize: ${normalizeTime}ms (${iterations * testPaths.length} ops, ${((iterations * testPaths.length) / normalizeTime).toFixed(0)} ops/ms)`);

// Benchmark resolve
start = Date.now();
for (let i = 0; i < iterations; i++) {
  for (const p of testPaths) {
    path.resolve(p);
  }
}
end = Date.now();
const resolveTime = end - start;
console.log(`resolve: ${resolveTime}ms (${iterations * testPaths.length} ops, ${((iterations * testPaths.length) / resolveTime).toFixed(0)} ops/ms)`);

// Benchmark isAbsolute
start = Date.now();
for (let i = 0; i < iterations; i++) {
  for (const p of testPaths) {
    path.isAbsolute(p);
  }
}
end = Date.now();
const isAbsoluteTime = end - start;
console.log(`isAbsolute: ${isAbsoluteTime}ms (${iterations * testPaths.length} ops, ${((iterations * testPaths.length) / isAbsoluteTime).toFixed(0)} ops/ms)`);

// Benchmark relative
start = Date.now();
for (let i = 0; i < iterations; i++) {
  path.relative('/home/user', '/home/user/documents/file.txt');
}
end = Date.now();
const relativeTime = end - start;
console.log(`relative: ${relativeTime}ms (${iterations} ops, ${(iterations / relativeTime).toFixed(0)} ops/ms)`);

// Benchmark join
start = Date.now();
for (let i = 0; i < iterations; i++) {
  path.join('/home', 'user', 'documents', 'file.txt');
}
end = Date.now();
const joinTime = end - start;
console.log(`join: ${joinTime}ms (${iterations} ops, ${(iterations / joinTime).toFixed(0)} ops/ms)`);

const totalTime = dirnameTime + basenameTime + extnameTime + normalizeTime +
                  resolveTime + isAbsoluteTime + relativeTime + joinTime;

console.log(`\nTotal time: ${totalTime}ms`);
console.log(`Operations per second: ${((iterations * testPaths.length * 6 + iterations * 2) / (totalTime / 1000)).toFixed(0)}`);
