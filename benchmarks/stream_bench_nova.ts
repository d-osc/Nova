// Nova Stream Benchmark
import { Readable, Writable, Transform, pipeline } from 'nova:stream';

const CHUNK_SIZE = 16384; // 16KB
const TOTAL_DATA = 100 * 1024 * 1024; // 100MB
const NUM_CHUNKS = TOTAL_DATA / CHUNK_SIZE;

console.log('=== Nova Stream Benchmark ===');
console.log(`Chunk size: ${CHUNK_SIZE} bytes`);
console.log(`Total data: ${TOTAL_DATA / 1024 / 1024} MB`);
console.log(`Number of chunks: ${NUM_CHUNKS}`);

// Test 1: Readable Stream Performance
console.log('\n[Test 1] Readable Stream Performance');
const startRead = Date.now();
let readCount = 0;

class FastReadable extends Readable {
    private chunksSent = 0;

    _read() {
        if (this.chunksSent < NUM_CHUNKS) {
            const chunk = Buffer.alloc(CHUNK_SIZE);
            this.push(chunk);
            this.chunksSent++;
        } else {
            this.push(null);
        }
    }
}

const readable = new FastReadable();
readable.on('data', (chunk) => {
    readCount += chunk.length;
});

readable.on('end', () => {
    const endRead = Date.now();
    const duration = (endRead - startRead) / 1000;
    const throughput = (readCount / 1024 / 1024) / duration;
    console.log(`Read ${readCount / 1024 / 1024} MB in ${duration.toFixed(2)}s`);
    console.log(`Throughput: ${throughput.toFixed(2)} MB/s`);

    // Test 2: Writable Stream Performance
    testWritable();
});

// Test 2: Writable Stream Performance
function testWritable() {
    console.log('\n[Test 2] Writable Stream Performance');
    const startWrite = Date.now();
    let writeCount = 0;

    class FastWritable extends Writable {
        _write(chunk: Buffer, encoding: string, callback: Function) {
            writeCount += chunk.length;
            callback();
        }
    }

    const writable = new FastWritable();

    writable.on('finish', () => {
        const endWrite = Date.now();
        const duration = (endWrite - startWrite) / 1000;
        const throughput = (writeCount / 1024 / 1024) / duration;
        console.log(`Wrote ${writeCount / 1024 / 1024} MB in ${duration.toFixed(2)}s`);
        console.log(`Throughput: ${throughput.toFixed(2)} MB/s`);

        // Test 3: Transform Stream Performance
        testTransform();
    });

    // Write chunks
    for (let i = 0; i < NUM_CHUNKS; i++) {
        const chunk = Buffer.alloc(CHUNK_SIZE);
        writable.write(chunk);
    }
    writable.end();
}

// Test 3: Transform Stream Performance
function testTransform() {
    console.log('\n[Test 3] Transform Stream Performance');
    const startTransform = Date.now();
    let transformCount = 0;

    class FastTransform extends Transform {
        _transform(chunk: Buffer, encoding: string, callback: Function) {
            transformCount += chunk.length;
            this.push(chunk);
            callback();
        }
    }

    const source = new FastReadable();
    const transform = new FastTransform();
    let outputCount = 0;

    transform.on('data', (chunk) => {
        outputCount += chunk.length;
    });

    transform.on('end', () => {
        const endTransform = Date.now();
        const duration = (endTransform - startTransform) / 1000;
        const throughput = (transformCount / 1024 / 1024) / duration;
        console.log(`Transformed ${transformCount / 1024 / 1024} MB in ${duration.toFixed(2)}s`);
        console.log(`Throughput: ${throughput.toFixed(2)} MB/s`);

        // Test 4: Pipe Performance
        testPipe();
    });

    source.pipe(transform);
}

// Test 4: Pipe Performance
function testPipe() {
    console.log('\n[Test 4] Pipe Performance');
    const startPipe = Date.now();
    let pipeCount = 0;

    const source = new FastReadable();
    const writable = new FastWritable();

    writable._write = function(chunk: Buffer, encoding: string, callback: Function) {
        pipeCount += chunk.length;
        callback();
    };

    writable.on('finish', () => {
        const endPipe = Date.now();
        const duration = (endPipe - startPipe) / 1000;
        const throughput = (pipeCount / 1024 / 1024) / duration;
        console.log(`Piped ${pipeCount / 1024 / 1024} MB in ${duration.toFixed(2)}s`);
        console.log(`Throughput: ${throughput.toFixed(2)} MB/s`);

        console.log('\n=== Benchmark Complete ===');
    });

    source.pipe(writable);
}
