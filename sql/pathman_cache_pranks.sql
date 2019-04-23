\set VERBOSITY terse
-- is pathman (caches, in particular) strong enough to carry out this?

SET search_path = 'public';

-- make sure nothing breaks on disable/enable when nothing was initialized yet
SET pg_pathman.enable = false;
SET pg_pathman.enable = true;

-- wobble with create-drop ext: tests cached relids sanity
CREATE EXTENSION pg_pathman;
SET pg_pathman.enable = f;
DROP EXTENSION pg_pathman;
CREATE EXTENSION pg_pathman;
SET pg_pathman.enable = true;
DROP EXTENSION pg_pathman;
CREATE EXTENSION pg_pathman;
DROP EXTENSION pg_pathman;

-- create it for further tests
CREATE EXTENSION pg_pathman;

-- 079797e0d5
CREATE TABLE part_test(val serial);
INSERT INTO part_test SELECT generate_series(1, 30);
SELECT create_range_partitions('part_test', 'val', 1, 10);
SELECT set_interval('part_test', 100);
DELETE FROM pathman_config WHERE partrel = 'part_test'::REGCLASS;
SELECT drop_partitions('part_test');
SELECT disable_pathman_for('part_test');

CREATE TABLE wrong_partition (LIKE part_test) INHERITS (part_test);
SELECT add_to_pathman_config('part_test', 'val', '10');
SELECT add_to_pathman_config('part_test', 'val');

DROP TABLE part_test CASCADE;
--

-- 85fc5ccf121
CREATE TABLE part_test(val serial);
INSERT INTO part_test SELECT generate_series(1, 3000);
SELECT create_range_partitions('part_test', 'val', 1, 10);
SELECT append_range_partition('part_test');
DELETE FROM part_test;
SELECT create_single_range_partition('part_test', NULL::INT4, NULL);	/* not ok */
DELETE FROM pathman_config WHERE partrel = 'part_test'::REGCLASS;
SELECT create_hash_partitions('part_test', 'val', 2, partition_names := ARRAY[]::TEXT[]); /* not ok */

DROP TABLE part_test CASCADE;
--

-- finalize
DROP EXTENSION pg_pathman;
