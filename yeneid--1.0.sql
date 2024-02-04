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


CREATE OR REPLACE FUNCTION yeneid_define_relation_offload_policy_internal(reloid OID) RETURNS void
AS 'MODULE_PATHNAME'
VOLATILE
EXECUTE ON COORDINATOR
LANGUAGE C STRICT;


CREATE OR REPLACE FUNCTION yeneid_define_relation_offload_policy_internal_seg(reloid OID) RETURNS void
AS 'MODULE_PATHNAME'
VOLATILE
EXECUTE ON ALL SEGMENTS
LANGUAGE C STRICT;

