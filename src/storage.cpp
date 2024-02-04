#include "ystorage.h"
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

static std::unordered_map<Oid, YeneidMetadataState> cache_;
YeneidMetadataState *get_yeneid_metadata(const Oid relationOid) {
  return &cache_[relationOid];
}


void init_yeneid_metadata(const Oid relationOid, SMgrRelation smgr) {
   cache_[relationOid] = YeneidMetadataState(relationOid, smgr);
}

void cleanup_yeneid_metadata(const Oid relationOid) {
   cache_.erase(relationOid);
}

char * yeneid_get_page_insert_ptr(char * buf, int page_size, int sz) {
  char * curr_ptr = buf;
  char * end_page_ptr = buf + page_size;

  while (1) {
    if (curr_ptr + 4 > end_page_ptr) {
      // failed to plase tuple on page
      break;
    }
    if (!memtuple_lead_bit_set((MemTuple)curr_ptr)) {
      return curr_ptr;
    }
    int nsz = memtuple_get_size((MemTuple)curr_ptr);
    if (nsz == 0) {
      return curr_ptr;
    }
    curr_ptr += nsz;
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

  auto h = get_yeneid_metadata(RelationGetRelid(relation));

  /*
   * get space to insert our next item (tuple)
   */
  auto itemLen = memtuple_get_size(memtup);


  if (h->tuple_place_ptr == nullptr) {
    if (h->npagecnt == 0) {
      memset(h->buf, 0, sizeof h->buf);
      smgrextend(h->smgr, MAIN_FORKNUM, 0, h->buf, false);
      h->tuple_place_ptr = h->buf;
    } else {
      smgrread(h->smgr, MAIN_FORKNUM, h->npagecnt  - 1, h->buf);
      // search for place for tuple
      h->tuple_place_ptr = yeneid_get_page_insert_ptr(h->buf, BLCKSZ, itemLen);
    }
  }

  if (h->tuple_place_ptr + itemLen > h->buf + BLCKSZ) {
    if (h->npagecnt) {
      smgrwrite(h->smgr, MAIN_FORKNUM,  h->npagecnt  - 1, h->buf, false);
    }

    ++h->npagecnt;
    memset(h->buf, 0, sizeof h->buf);
    h->tuple_place_ptr = h->buf;
  }

  memcpy(h->tuple_place_ptr,(char*)memtup, itemLen);
  h->tuple_place_ptr += itemLen;

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

    if (!memtuple_lead_bit_set((MemTuple)(scan->page_ptr))) {
      return false;
    }

    int sz = memtuple_get_size((MemTuple)(scan->page_ptr));

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


void yeneid_dml_init_internal(Relation relation) {
  RelationOpenSmgr(relation);
  init_yeneid_metadata(RelationGetRelid(relation), relation->rd_smgr);
}


void yeneid_dml_finish_internal(Relation relation) {
  auto h = get_yeneid_metadata(RelationGetRelid(relation));
  if (h->npagecnt) {
    smgrwrite(h->smgr, MAIN_FORKNUM,  h->npagecnt - 1, h->buf, false);
  }

  cleanup_yeneid_metadata(RelationGetRelid(relation));
}