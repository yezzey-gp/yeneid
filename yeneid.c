/*--------------------------------------------------------------------------
 *
 * yeneid.c
 *	  Yeneid access methods
 *
 *
 * IDENTIFICATION
 *	    yeneid.c
 *
 *--------------------------------------------------------------------------
 */

#include "postgres.h"

#include <math.h>

#include "miscadmin.h"

#include "access/amapi.h"
#include "access/heapam.h"
#include "access/tableam.h"
#include "catalog/index.h"
#include "commands/vacuum.h"
#include "executor/tuptable.h"

#include "yeneid.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(yeneid_handler);

static const TableAmRoutine yeneid_methods;

/* ------------------------------------------------------------------------
 * Slot related callbacks for yeneid AM
 * ------------------------------------------------------------------------
 */

static const TupleTableSlotOps *yeneid_slot_callbacks(Relation relation) {
  /*
   * Here you would most likely want to invent your own set of slot
   * callbacks for your AM.
   */
  return &TTSOpsMinimalTuple;
}

/* ------------------------------------------------------------------------
 * Table Scan Callbacks for yeneid AM
 * ------------------------------------------------------------------------
 */

static TableScanDesc yeneid_scan_begin(Relation relation, Snapshot snapshot,
                                       int nkeys, ScanKey key,
                                       ParallelTableScanDesc parallel_scan,
                                       uint32 flags) {
  YeneidScanDesc scan;

  scan = (YeneidScanDesc)palloc0(sizeof(YeneidScanDescData));

  scan->rs_base.rs_rd = relation;
  scan->rs_base.rs_snapshot = snapshot;
  scan->rs_base.rs_nkeys = nkeys;
  scan->rs_base.rs_flags = flags;
  scan->rs_base.rs_parallel = parallel_scan;

  yeneid_scan_init(scan);

  return (TableScanDesc)scan;
}

static void yeneid_scan_end(TableScanDesc sscan) {
  YeneidScanDesc scan = (YeneidScanDesc)sscan;

  pfree(scan);
}

static void yeneid_scan_rescan(TableScanDesc sscan, ScanKey key,
                               bool set_params, bool allow_strat,
                               bool allow_sync, bool allow_pagemode) {
  /* nothing to do */
}

static bool yeneid_scan_getnextslot(TableScanDesc sscan,
                                    ScanDirection direction,
                                    TupleTableSlot *slot) {
  /* nothing to do */
  return yeneid_scan_getnextslot_internal((YeneidScanDesc)sscan, direction,
                                          slot);
}

/* ------------------------------------------------------------------------
 * Index Scan Callbacks for yeneid AM
 * ------------------------------------------------------------------------
 */

static IndexFetchTableData *yeneid_index_fetch_begin(Relation rel) {
  return NULL;
}

static void yeneid_index_fetch_reset(IndexFetchTableData *scan) {
  /* nothing to do here */
}

static void yeneid_index_fetch_end(IndexFetchTableData *scan) {
  /* nothing to do here */
}

static bool yeneid_index_fetch_tuple(struct IndexFetchTableData *scan,
                                     ItemPointer tid, Snapshot snapshot,
                                     TupleTableSlot *slot, bool *call_again,
                                     bool *all_dead) {
  /* there is no data */
  return 0;
}

/* ------------------------------------------------------------------------
 * Callbacks for non-modifying operations on individual tuples for
 * yeneid AM.
 * ------------------------------------------------------------------------
 */

static bool yeneid_fetch_row_version(Relation relation, ItemPointer tid,
                                     Snapshot snapshot, TupleTableSlot *slot) {
  /* nothing to do */
  return false;
}

static void yeneid_get_latest_tid(TableScanDesc sscan, ItemPointer tid) {
  /* nothing to do */
}

static bool yeneid_tuple_tid_valid(TableScanDesc scan, ItemPointer tid) {
  return false;
}

static bool yeneid_tuple_satisfies_snapshot(Relation rel, TupleTableSlot *slot,
                                            Snapshot snapshot) {
  return false;
}

#if PG_VERSION_NUM >= 140000

static TransactionId yeneid_index_delete_tuples(Relation rel,
                                                TM_IndexDeleteOp *delstate) {
  return InvalidTransactionId;
}

#endif

/* ----------------------------------------------------------------------------
 *  Functions for manipulations of physical tuples for yeneid AM.
 * ----------------------------------------------------------------------------
 */

static void yeneid_tuple_insert(Relation relation, TupleTableSlot *slot,
                                CommandId cid, int options,
                                BulkInsertState bistate) {
  /* nothing to do */

  (void)yeneid_tuple_insert_internal(relation, slot, cid, options, bistate);
}

static void yeneid_tuple_insert_speculative(Relation relation,
                                            TupleTableSlot *slot, CommandId cid,
                                            int options,
                                            BulkInsertState bistate,
                                            uint32 specToken) {
  /* nothing to do */
}

static void yeneid_tuple_complete_speculative(Relation relation,
                                              TupleTableSlot *slot,
                                              uint32 spekToken,
                                              bool succeeded) {
  /* nothing to do */
}

static void yeneid_multi_insert(Relation relation, TupleTableSlot **slots,
                                int ntuples, CommandId cid, int options,
                                BulkInsertState bistate) {
	for (int i = 0; i < ntuples; i++)
		yeneid_tuple_insert_internal(relation, slots[i], cid, options, bistate);
}

static TM_Result yeneid_tuple_delete(Relation relation, ItemPointer tid,
                                     CommandId cid, Snapshot snapshot,
                                     Snapshot crosscheck, bool wait,
                                     TM_FailureData *tmfd, bool changingPart) {
  /* nothing to do, so it is always OK */
  return TM_Ok;
}

#if PG_VERSION_NUM >= 130000

static TM_Result yeneid_tuple_update(Relation relation, ItemPointer otid,
                                     TupleTableSlot *slot, CommandId cid,
                                     Snapshot snapshot, Snapshot crosscheck,
                                     bool wait, TM_FailureData *tmfd,
                                     LockTupleMode *lockmode,
                                     TU_UpdateIndexes *update_indexes) {
  /* nothing to do, so it is always OK */
  return TM_Ok;
}

#else

/* see table_tuple_update() for reference about parameters */
static TM_Result yeneid_tuple_update(Relation rel, ItemPointer otid,
                                     TupleTableSlot *slot, CommandId cid,
                                     Snapshot snapshot, Snapshot crosscheck,
                                     bool wait, TM_FailureData *tmfd,
                                     LockTupleMode *lockmode,
                                     bool *update_indexes) {
  /* nothing to do, so it is always OK */
  return TM_Ok;
}

#endif

static TM_Result yeneid_tuple_lock(Relation relation, ItemPointer tid,
                                   Snapshot snapshot, TupleTableSlot *slot,
                                   CommandId cid, LockTupleMode mode,
                                   LockWaitPolicy wait_policy, uint8 flags,
                                   TM_FailureData *tmfd) {
  /* nothing to do, so it is always OK */
  return TM_Ok;
}

static void yeneid_finish_bulk_insert(Relation relation, int options) {
  /* nothing to do */
}

/* ------------------------------------------------------------------------
 * DDL related callbacks for yeneid AM.
 * ------------------------------------------------------------------------
 */

#if PG_VERSION_NUM >= 160000
static void yeneid_relation_set_new_filelocator(Relation rel,
                                                const RelFileLocator *newrnode,
                                                char persistence,
                                                TransactionId *freezeXid,
                                                MultiXactId *minmulti) {
  /* nothing to do */
}
#else

static void yeneid_relation_set_new_relfilenode(Relation rel,
                                                const RelFileNode *newrnode,
                                                char persistence,
                                                TransactionId *freezeXid,
                                                MultiXactId *minmulti) {
  yeneid_relation_set_new_relfilenode_internal(rel, newrnode, persistence, freezeXid, minmulti);
}

#endif

static void yeneid_relation_nontransactional_truncate(Relation rel) {
  /* nothing to do */
}

#if PG_VERSION_NUM >= 160000

static void yeneid_copy_data(Relation rel, const RelFileLocator *newrnode) {
  /* there is no data */
}

#else

static void yeneid_copy_data(Relation rel, const RelFileNode *newrnode) {
  /* there is no data */
}

#endif

static void yeneid_copy_for_cluster(Relation OldTable, Relation NewTable,
                                    Relation OldIndex, bool use_sort,
                                    TransactionId OldestXmin,
                                    TransactionId *xid_cutoff,
                                    MultiXactId *multi_cutoff,
                                    double *num_tuples, double *tups_vacuumed,
                                    double *tups_recently_dead) {
  /* no data, so nothing to do */
}

static void yeneid_vacuum(Relation onerel, VacuumParams *params,
                          BufferAccessStrategy bstrategy) {
  /* no data, so nothing to do */
}

static bool yeneid_scan_analyze_next_block(TableScanDesc scan,
                                           BlockNumber blockno,
                                           BufferAccessStrategy bstrategy) {
  /* no data, so no point to analyze next block */
  return false;
}

static bool yeneid_scan_analyze_next_tuple(TableScanDesc scan,
                                           TransactionId OldestXmin,
                                           double *liverows, double *deadrows,
                                           TupleTableSlot *slot) {
  /* no data, so no point to analyze next tuple */
  return false;
}

static double yeneid_index_build_range_scan(
    Relation tableRelation, Relation indexRelation, IndexInfo *indexInfo,
    bool allow_sync, bool anyvisible, bool progress, BlockNumber start_blockno,
    BlockNumber numblocks, IndexBuildCallback callback, void *callback_state,
    TableScanDesc scan) {
  /* no data, so no tuples */
  return 0;
}

static void yeneid_index_validate_scan(Relation tableRelation,
                                       Relation indexRelation,
                                       IndexInfo *indexInfo, Snapshot snapshot,
                                       ValidateIndexState *state) {
  /* nothing to do */
}

/* ------------------------------------------------------------------------
 * Miscellaneous callbacks for the yeneid AM
 * ------------------------------------------------------------------------
 */

static uint64 yeneid_relation_size(Relation rel, ForkNumber forkNumber) {
  /* there is nothing */
  return 0;
}

/*
 * Check to see whether the table needs a TOAST table.
 */
static bool yeneid_relation_needs_toast_table(Relation rel) {
  /* no data, so no toast table needed */
  return false;
}

/* ------------------------------------------------------------------------
 * Planner related callbacks for the yeneid AM
 * ------------------------------------------------------------------------
 */

static void yeneid_estimate_rel_size(Relation rel, int32 *attr_widths,
                                     BlockNumber *pages, double *tuples,
                                     double *allvisfrac) {
  /* no data available */
  if (attr_widths)
    *attr_widths = 0;
  if (pages)
    *pages = 0;
  if (tuples)
    *tuples = 0;
  if (allvisfrac)
    *allvisfrac = 0;
}

/* ------------------------------------------------------------------------
 * Executor related callbacks for the yeneid AM
 * ------------------------------------------------------------------------
 */

static bool yeneid_scan_bitmap_next_block(TableScanDesc scan,
                                          TBMIterateResult *tbmres) {
  /* no data, so no point to scan next block */
  return false;
}

static bool yeneid_scan_bitmap_next_tuple(TableScanDesc scan,
                                          TBMIterateResult *tbmres,
                                          TupleTableSlot *slot) {
  /* no data, so no point to scan next tuple */
  return false;
}

static bool yeneid_scan_sample_next_block(TableScanDesc scan,
                                          SampleScanState *scanstate) {
  /* no data, so no point to scan next block for sampling */
  return false;
}

static bool yeneid_scan_sample_next_tuple(TableScanDesc scan,
                                          SampleScanState *scanstate,
                                          TupleTableSlot *slot) {
  /* no data, so no point to scan next tuple for sampling */
  return false;
}

#if PG_VERSION_NUM < 140000
static TransactionId
yeneid_compute_xid_horizon_for_tuples(Relation rel, ItemPointerData *tids,
                                      int nitems) {
  /*
   * This API is only useful for hot standby snapshot conflict resolution
   * (for eg. see btree_xlog_delete()), in the context of index page-level
   * vacuums (aka page-level cleanups). This operation is only done when
   * IndexScanDesc->kill_prior_tuple is true, which is never for AO/CO tables
   * (we always return all_dead = false in the index_fetch_tuple() callback
   * as we don't support HOT)
   */
  ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                  errmsg("feature not supported on yeneid relations")));
}
#endif

#ifdef GP_VERSION_NUM

static void yeneid_relation_add_columns(Relation rel, List *newvals,
                                        List *constraints, TupleDesc oldDesc) {
  elog(ERROR, "add columns-only not implemented for yeneid tables");
}

static void yeneid_relation_rewrite_columns(Relation rel, List *newvals,
                                            TupleDesc oldDesc) {
  elog(ERROR, "rewrite columns-only not implemented for yeneid tables");
}

/*
 * For each AO segment, get the starting heap block number and the number of
 * heap blocks (together termed as a BlockSequence). The starting heap block
 * number is always deterministic given a segment number. See AOtupleId.
 *
 * The number of heap blocks can be determined from the last row number present
 * in the segment. See appendonlytid.h for details.
 */
static BlockSequence *yeneid_relation_get_block_sequences(Relation rel,
                                                          int *numSequences) {
  elog(ERROR, "not implemented for yeneid tables");
}
/*
 * Return the BlockSequence corresponding to the AO segment in which the logical
 * heap block 'blkNum' falls.
 */
static void yeneid_relation_get_block_sequence(Relation rel, BlockNumber blkNum,
                                               BlockSequence *sequence) {
  elog(ERROR, "not implemented for yeneid tables");
}

/*
 * Provides an opportunity to create backend-local state to be consulted during
 * the course of the current DML or DML-like command, for the given relation.
 */
static void yeneid_dml_init(Relation relation) {
  yeneid_dml_init_internal(relation);
}

/*
 * Provides an opportunity to clean up backend-local state set up for the
 * current DML or DML-like command, for the given relation.
 */
static void yeneid_dml_finish(Relation relation) {
  yeneid_dml_finish_internal(relation);
}

#endif

/* ------------------------------------------------------------------------
 * Definition of the yeneid table access method.
 * ------------------------------------------------------------------------
 */

static const TableAmRoutine yeneid_methods = {
    .type = T_TableAmRoutine,

    .slot_callbacks = yeneid_slot_callbacks,

    .scan_begin = yeneid_scan_begin,
    .scan_end = yeneid_scan_end,
    .scan_rescan = yeneid_scan_rescan,
    .scan_getnextslot = yeneid_scan_getnextslot,

    /* these are common helper functions */
    .parallelscan_estimate = table_block_parallelscan_estimate,
    .parallelscan_initialize = table_block_parallelscan_initialize,
    .parallelscan_reinitialize = table_block_parallelscan_reinitialize,

    .index_fetch_begin = yeneid_index_fetch_begin,
    .index_fetch_reset = yeneid_index_fetch_reset,
    .index_fetch_end = yeneid_index_fetch_end,
    .index_fetch_tuple = yeneid_index_fetch_tuple,

    .tuple_insert = yeneid_tuple_insert,
    .tuple_insert_speculative = yeneid_tuple_insert_speculative,
    .tuple_complete_speculative = yeneid_tuple_complete_speculative,
    .multi_insert = yeneid_multi_insert,
    .tuple_delete = yeneid_tuple_delete,
    .tuple_update = yeneid_tuple_update,
    .tuple_lock = yeneid_tuple_lock,
    .finish_bulk_insert = yeneid_finish_bulk_insert,

    .tuple_fetch_row_version = yeneid_fetch_row_version,
    .tuple_get_latest_tid = yeneid_get_latest_tid,
    .tuple_tid_valid = yeneid_tuple_tid_valid,
    .tuple_satisfies_snapshot = yeneid_tuple_satisfies_snapshot,

#if PG_VERSION_NUM < 140000
    .compute_xid_horizon_for_tuples = yeneid_compute_xid_horizon_for_tuples,
#endif

#if PG_VERSION_NUM >= 140000
    .index_delete_tuples = yeneid_index_delete_tuples,
#endif

#if PG_VERSION_NUM >= 160000
    .relation_set_new_filelocator = yeneid_relation_set_new_filelocator,
#else
    .relation_set_new_filenode = yeneid_relation_set_new_relfilenode,
#endif

    .relation_nontransactional_truncate =
        yeneid_relation_nontransactional_truncate,
    .relation_copy_data = yeneid_copy_data,
    .relation_copy_for_cluster = yeneid_copy_for_cluster,
    .relation_vacuum = yeneid_vacuum,
    .scan_analyze_next_block = yeneid_scan_analyze_next_block,
    .scan_analyze_next_tuple = yeneid_scan_analyze_next_tuple,
    .index_build_range_scan = yeneid_index_build_range_scan,
    .index_validate_scan = yeneid_index_validate_scan,

    .relation_size = yeneid_relation_size,

#ifdef GP_VERSION_NUM
    .relation_add_columns = yeneid_relation_add_columns,
    .relation_rewrite_columns = yeneid_relation_rewrite_columns,
    .relation_get_block_sequences = yeneid_relation_get_block_sequences,
    .relation_get_block_sequence = yeneid_relation_get_block_sequence,
    .dml_init = yeneid_dml_init,
    .dml_finish = yeneid_dml_finish,
#endif

    .relation_needs_toast_table = yeneid_relation_needs_toast_table,

    .relation_estimate_size = yeneid_estimate_rel_size,

    .scan_bitmap_next_block = yeneid_scan_bitmap_next_block,
    .scan_bitmap_next_tuple = yeneid_scan_bitmap_next_tuple,
    .scan_sample_next_block = yeneid_scan_sample_next_block,
    .scan_sample_next_tuple = yeneid_scan_sample_next_tuple};

Datum yeneid_handler(PG_FUNCTION_ARGS) { PG_RETURN_POINTER(&yeneid_methods); }
