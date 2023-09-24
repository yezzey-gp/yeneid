DROP DATABASE yeneid;
CREATE DATABASE yeneid;

\c yeneid

CREATE EXTENSION yeneid;

CREATE TABLE t(i int) USING yeneid;
