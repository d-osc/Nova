// SQLite Performance Benchmark - Nova vs Node.js
// Compares standard SQLite with ultra-optimized version

import { DatabaseSync } from 'node:sqlite';

// ========================================
// Benchmark 1: Batch Insert (10,000 rows)
// ========================================
function benchmarkBatchInsert() {
  console.log('\n=== Benchmark 1: Batch Insert (10,000 rows) ===');

  const db = new DatabaseSync(':memory:');

  db.exec(`
    CREATE TABLE users (
      id INTEGER PRIMARY KEY,
      name TEXT NOT NULL,
      email TEXT NOT NULL,
      age INTEGER,
      created_at INTEGER
    )
  `);

  const stmt = db.prepare('INSERT INTO users (name, email, age, created_at) VALUES (?, ?, ?, ?)');

  // Without transaction
  console.time('  Without transaction');
  for (let i = 0; i < 1000; i++) {
    stmt.run(`User ${i}`, `user${i}@example.com`, 20 + (i % 50), Date.now());
  }
  console.timeEnd('  Without transaction');

  // With transaction
  console.time('  With transaction');
  db.exec('BEGIN TRANSACTION');
  for (let i = 1000; i < 10000; i++) {
    stmt.run(`User ${i}`, `user${i}@example.com`, 20 + (i % 50), Date.now());
  }
  db.exec('COMMIT');
  console.timeEnd('  With transaction');

  db.close();
}

// ========================================
// Benchmark 2: Repeated Queries (1,000 times)
// ========================================
function benchmarkRepeatedQueries() {
  console.log('\n=== Benchmark 2: Repeated Queries (1,000 executions) ===');

  const db = new DatabaseSync(':memory:');

  db.exec(`
    CREATE TABLE products (
      id INTEGER PRIMARY KEY,
      name TEXT,
      price REAL,
      category TEXT
    )
  `);

  // Insert test data
  const insert = db.prepare('INSERT INTO products (name, price, category) VALUES (?, ?, ?)');
  db.exec('BEGIN');
  for (let i = 0; i < 100; i++) {
    insert.run(`Product ${i}`, 10.99 + i, i % 5 === 0 ? 'Electronics' : 'Books');
  }
  db.exec('COMMIT');

  // Same query repeated
  const query = db.prepare('SELECT * FROM products WHERE id = ?');

  console.time('  1,000 queries');
  for (let i = 0; i < 1000; i++) {
    query.all(i % 100);
  }
  console.timeEnd('  1,000 queries');

  db.close();
}

// ========================================
// Benchmark 3: Large Result Set (100,000 rows)
// ========================================
function benchmarkLargeResultSet() {
  console.log('\n=== Benchmark 3: Large Result Set (100,000 rows) ===');

  const db = new DatabaseSync(':memory:');

  db.exec(`
    CREATE TABLE logs (
      id INTEGER PRIMARY KEY,
      timestamp INTEGER,
      level TEXT,
      message TEXT
    )
  `);

  // Insert 100,000 rows
  console.time('  Insert 100k rows');
  const stmt = db.prepare('INSERT INTO logs (timestamp, level, message) VALUES (?, ?, ?)');
  db.exec('BEGIN');
  for (let i = 0; i < 100000; i++) {
    stmt.run(
      Date.now() - i * 1000,
      i % 3 === 0 ? 'ERROR' : i % 3 === 1 ? 'WARN' : 'INFO',
      `Log message ${i} with some additional text to simulate real logs`
    );
  }
  db.exec('COMMIT');
  console.timeEnd('  Insert 100k rows');

  // Query all rows
  console.time('  Query all 100k rows');
  const rows = db.prepare('SELECT * FROM logs ORDER BY timestamp DESC').all();
  console.timeEnd('  Query all 100k rows');
  console.log(`  Retrieved ${rows.length} rows`);

  db.close();
}

// ========================================
// Benchmark 4: Complex Queries with Joins
// ========================================
function benchmarkComplexQueries() {
  console.log('\n=== Benchmark 4: Complex Queries with Joins ===');

  const db = new DatabaseSync(':memory:');

  db.exec(`
    CREATE TABLE orders (
      id INTEGER PRIMARY KEY,
      user_id INTEGER,
      total REAL,
      status TEXT
    );

    CREATE TABLE order_items (
      id INTEGER PRIMARY KEY,
      order_id INTEGER,
      product_id INTEGER,
      quantity INTEGER,
      price REAL
    );

    CREATE INDEX idx_orders_user ON orders(user_id);
    CREATE INDEX idx_items_order ON order_items(order_id);
  `);

  // Insert test data
  console.time('  Insert test data');
  db.exec('BEGIN');
  const orderStmt = db.prepare('INSERT INTO orders (user_id, total, status) VALUES (?, ?, ?)');
  const itemStmt = db.prepare('INSERT INTO order_items (order_id, product_id, quantity, price) VALUES (?, ?, ?, ?)');

  for (let i = 1; i <= 1000; i++) {
    orderStmt.run(i % 100, 50.0 + i, i % 3 === 0 ? 'completed' : 'pending');

    // 3-5 items per order
    const itemCount = 3 + (i % 3);
    for (let j = 0; j < itemCount; j++) {
      itemStmt.run(i, j + 1, 1 + (j % 3), 10.99 * (j + 1));
    }
  }
  db.exec('COMMIT');
  console.timeEnd('  Insert test data');

  // Complex join query
  const joinQuery = db.prepare(`
    SELECT
      o.id as order_id,
      o.user_id,
      o.total,
      o.status,
      COUNT(oi.id) as item_count,
      SUM(oi.quantity) as total_items,
      SUM(oi.price * oi.quantity) as calculated_total
    FROM orders o
    LEFT JOIN order_items oi ON o.id = oi.order_id
    WHERE o.user_id = ?
    GROUP BY o.id
    ORDER BY o.id DESC
  `);

  console.time('  100 complex queries');
  for (let i = 0; i < 100; i++) {
    joinQuery.all(i % 100);
  }
  console.timeEnd('  100 complex queries');

  db.close();
}

// ========================================
// Benchmark 5: Prepared Statement Reuse
// ========================================
function benchmarkStatementReuse() {
  console.log('\n=== Benchmark 5: Prepared Statement Reuse (Statement Caching) ===');

  const db = new DatabaseSync(':memory:');

  db.exec(`
    CREATE TABLE metrics (
      id INTEGER PRIMARY KEY,
      metric_name TEXT,
      value REAL,
      timestamp INTEGER
    )
  `);

  // Insert test data
  const insert = db.prepare('INSERT INTO metrics (metric_name, value, timestamp) VALUES (?, ?, ?)');
  db.exec('BEGIN');
  for (let i = 0; i < 1000; i++) {
    insert.run(`metric_${i % 10}`, Math.random() * 100, Date.now());
  }
  db.exec('COMMIT');

  // Reuse same prepared statement 10,000 times
  const query = db.prepare('SELECT COUNT(*) as count, AVG(value) as avg FROM metrics WHERE metric_name = ?');

  console.time('  10,000 statement reuses');
  for (let i = 0; i < 10000; i++) {
    query.all(`metric_${i % 10}`);
  }
  console.timeEnd('  10,000 statement reuses');

  db.close();
}

// ========================================
// Benchmark 6: Memory Usage Test
// ========================================
function benchmarkMemoryUsage() {
  console.log('\n=== Benchmark 6: Memory Usage (Large Strings) ===');

  const db = new DatabaseSync(':memory:');

  db.exec(`
    CREATE TABLE documents (
      id INTEGER PRIMARY KEY,
      title TEXT,
      content TEXT,
      metadata TEXT
    )
  `);

  // Insert large text documents
  const largeText = 'Lorem ipsum dolor sit amet, '.repeat(100); // ~2.8KB per row
  const metadata = JSON.stringify({ tags: ['important', 'archived'], version: 1.0 });

  console.time('  Insert 10,000 documents');
  const stmt = db.prepare('INSERT INTO documents (title, content, metadata) VALUES (?, ?, ?)');
  db.exec('BEGIN');
  for (let i = 0; i < 10000; i++) {
    stmt.run(`Document ${i}`, largeText, metadata);
  }
  db.exec('COMMIT');
  console.timeEnd('  Insert 10,000 documents');

  // Query all
  console.time('  Query all 10,000 documents');
  const docs = db.prepare('SELECT * FROM documents').all();
  console.timeEnd('  Query all 10,000 documents');
  console.log(`  Retrieved ${docs.length} documents (~${Math.round(docs.length * 3 / 1024)} MB)`);

  db.close();
}

// ========================================
// Run All Benchmarks
// ========================================
console.log('========================================');
console.log('SQLite Performance Benchmark Suite');
console.log('========================================');

try {
  benchmarkBatchInsert();
  benchmarkRepeatedQueries();
  benchmarkLargeResultSet();
  benchmarkComplexQueries();
  benchmarkStatementReuse();
  benchmarkMemoryUsage();

  console.log('\n========================================');
  console.log('Benchmark Complete!');
  console.log('========================================');
} catch (error) {
  console.error('Benchmark failed:', error);
}
