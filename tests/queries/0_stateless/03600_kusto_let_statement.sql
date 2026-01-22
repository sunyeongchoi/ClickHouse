-- Test let statement with scalar values
SET allow_experimental_kusto_dialect = 1;
SET dialect = 'kusto';

let threshold = 100;
print threshold;

-- Test let statement with expressions
SET allow_experimental_kusto_dialect = 1;
SET dialect = 'kusto';
let x = 10;
let y = 20;
let sum = x + y;
print sum;

-- Test let statement used in filter
SET allow_experimental_kusto_dialect = 1;
SET dialect = 'kusto';
let limit_val = 3;
numbers(10) | where number < limit_val | project number;
