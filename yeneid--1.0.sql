/* yeneid/yeneid--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION yeneid" to load this file. \quit

CREATE FUNCTION yeneid_handler(internal)
RETURNS table_am_handler
AS 'MODULE_PATHNAME'
LANGUAGE C;

-- Access method
CREATE ACCESS METHOD yeneid TYPE TABLE HANDLER yeneid_handler;
COMMENT ON ACCESS METHOD yeneid IS 'table AM for Yezzey';

