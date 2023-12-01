#include "storage.h"
#include "yeneid.h"

/*
 * Utility code
 */

MemTuple yeneid_form_memtuple(TupleTableSlot *slot, MemTupleBinding *mt_bind) {
  MemTuple result;
  MemoryContext oldContext;

  /*
   * In case of a non virtal tuple, make certain that the slot's values are
   * populated, for example during a CTAS.
   */
  if (!TTS_IS_VIRTUAL(slot))
    slot_getsomeattrs(slot, slot->tts_tupleDescriptor->natts);

  oldContext = MemoryContextSwitchTo(slot->tts_mcxt);
  result = memtuple_form(mt_bind, slot->tts_values, slot->tts_isnull);
  MemoryContextSwitchTo(oldContext);

  return result;
}

/*
 * Retrieve the state information for a relation.
 * It is required that the state has been created before hand.
 */
YeneidMetadataState *get_yeneid_metadata(const Oid relationOid) {
  static std::unordered_map<Oid, YeneidMetadataState> cache_;
  if (!cache_.count(relationOid)) {
    // cache_[relationOid] = YeneidMetadataState(relationOid);
  }

  return &cache_[relationOid];
}

static int get_tuple_len(char * buf) {
  return (1 << 24) * buf[0] + (1 << 16) * buf[1] + (1 << 8) * buf[2] + buf[3];
}

char * yeneid_get_page_insert_ptr(char * buf, int page_size, int sz) {
  char * curr_ptr = buf;
  char * end_page_ptr = buf + page_size;

  while (1) {
    if (curr_ptr + 4 + sz > end_page_ptr) {
      // failed to plase tuple on page
      break;
    }
    int nsz = get_tuple_len(curr_ptr);
    if (nsz == 0) {
      return curr_ptr;
    }
    curr_ptr += 4 + nsz;
  }

  return nullptr;
}

void yeneid_tuple_insert_internal(Relation relation, TupleTableSlot *slot,
                                  CommandId cid, int options,
                                  BulkInsertState bistate) {
  /* cache binding */
  auto mt_bind = create_memtuple_binding(
      RelationGetDescr(relation), RelationGetNumberOfAttributes(relation));
  auto memtup = yeneid_form_memtuple(slot, mt_bind);


  RelationOpenSmgr(relation);
  auto h = YeneidMetadataState(RelationGetRelid(relation), relation->rd_smgr);

  /*
   * get space to insert our next item (tuple)
   */
  auto itemLen = memtuple_get_size(memtup);

  auto nblock = smgrnblocks(h.smgr, MAIN_FORKNUM);

  char buf[BLCKSZ];

  char *tuple_place_ptr = nullptr;

  if (nblock > 0) {
    smgrread(h.smgr, MAIN_FORKNUM, nblock - 1, buf);
    // search for place for tuple
    tuple_place_ptr = yeneid_get_page_insert_ptr(buf, BLCKSZ, itemLen);
  }

  if (tuple_place_ptr == nullptr) {
    ++nblock;
    memset(buf, 0, sizeof buf);
    tuple_place_ptr = buf;

    char bufsz[4] = {itemLen >> 24, (itemLen >> 16) & ((1 << 8) - 1), (itemLen >> 8) & ((1 << 8) - 1), itemLen & ((1 << 8) - 1)};
    memcpy(tuple_place_ptr, bufsz, 4);
    tuple_place_ptr += 4;
    memcpy(tuple_place_ptr,(char*)memtup, itemLen);

    smgrextend(h.smgr, MAIN_FORKNUM, nblock - 1, buf, false);
  } else {
    char bufsz[4] = {itemLen >> 24, (itemLen >> 16) & ((1 << 8) - 1), (itemLen >> 8) & ((1 << 8) - 1), itemLen & ((1 << 8) - 1)};
    memcpy(tuple_place_ptr, bufsz, 4);
    tuple_place_ptr += 4;
    memcpy(tuple_place_ptr,(char*)memtup, itemLen);

    smgrwrite(h.smgr, MAIN_FORKNUM, nblock - 1, buf, false);
  }

  pfree(memtup);
  pfree(mt_bind);
}

/* ----------------
 *		yeneid_getnextslot - retrieve next tuple in scan
 * ----------------
 */
bool yeneid_scan_getnextslot_internal(YeneidScanDesc scan,
                                      ScanDirection direction,
                                      TupleTableSlot *slot) {
  auto relation = scan->rs_base.rs_rd;
  YeneidMetadataState * h = (YeneidMetadataState *)(scan-> YeneidMetadataState);

  ItemPointerData fake_ctid;
  ExecClearTuple(slot);


  while (1) {
    if (scan->page_ptr == nullptr) {
      if (scan->current_block == h->npagecnt) {
        break;
      }
      smgrread(h->smgr, MAIN_FORKNUM, scan->current_block ++, scan->buf);
      scan->page_ptr = scan->buf;
    }

    if (scan->page_ptr + 4 > scan->buf + BLCKSZ) {
      scan->page_ptr == nullptr;
      continue;
    }

    int sz = get_tuple_len(scan->page_ptr);
    if (sz == 0) {
      return false;
    }
    scan->page_ptr += 4;


    auto mt_bind = create_memtuple_binding(
        RelationGetDescr(relation), RelationGetNumberOfAttributes(relation));

    memtuple_deform((MemTuple)(scan->page_ptr), mt_bind, slot->tts_values,
                    slot->tts_isnull);
    slot->tts_tid = fake_ctid;
    ExecStoreVirtualTuple(slot);

    pfree(mt_bind);

    scan->page_ptr += sz;

    return true;
  }

  return false;
}


void yeneid_scan_cleanup_internal(YeneidScanDesc scan) {
  delete (YeneidMetadataState*)scan->YeneidMetadataState;
}

void yeneid_scan_init(YeneidScanDesc scan) {
  RelationOpenSmgr(scan->rs_base.rs_rd);
  scan-> YeneidMetadataState = new YeneidMetadataState(RelationGetRelid(scan->rs_base.rs_rd), scan->rs_base.rs_rd->rd_smgr);
}




/* ------------------------------------------------------------------------
 * DDL related callbacks for heap AM.
 * ------------------------------------------------------------------------
 */

void
yeneid_relation_set_new_relfilenode_internal(Relation rel,
								 const RelFileNode *newrnode,
								 char persistence,
								 TransactionId *freezeXid,
								 MultiXactId *minmulti)
{
	SMgrRelation srel;

	/*
	 * Initialize to the minimum XID that could put tuples in the table. We
	 * know that no xacts older than RecentXmin are still running, so that
	 * will do.
	 */
	*freezeXid = RecentXmin;

	/*
	 * Similarly, initialize the minimum Multixact to the first value that
	 * could possibly be stored in tuples in the table.  Running transactions
	 * could reuse values from their local cache, so we are careful to
	 * consider all currently running multis.
	 *
	 * XXX this could be refined further, but is it worth the hassle?
	 */
	*minmulti = GetOldestMultiXactId();

	srel = RelationCreateStorage(*newrnode, persistence, SMGR_MD);

	smgrclose(srel);
}
