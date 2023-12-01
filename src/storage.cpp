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
    cache_[relationOid] = YeneidMetadataState(relationOid);
  }

  return &cache_[relationOid];
}

void yeneid_tuple_insert_internal(Relation relation, TupleTableSlot *slot,
                                  CommandId cid, int options,
                                  BulkInsertState bistate) {
  /* cache binding */
  auto mt_bind = create_memtuple_binding(
      RelationGetDescr(relation), RelationGetNumberOfAttributes(relation));
  auto memtup = yeneid_form_memtuple(slot, mt_bind);

  auto h = YeneidMetadataState(RelationGetRelid(relation));

  /*
   * get space to insert our next item (tuple)
   */
  auto itemLen = memtuple_get_size(memtup);

  h.os.seekp(0, std::ios::end);
  char buf[4] = {itemLen >> 24, (itemLen >> 16) & ((1 << 8) - 1), (itemLen >> 8) & ((1 << 8) - 1), itemLen & ((1 << 8) - 1)};
  h.os.write(buf, 4);
  h.os.write((char*)memtup, itemLen);
  elog(DEBUG5, "write up to %d bytes", h.os.tellp());
  h.os.flush();

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

  if (!h->is.eof()) {
    ItemPointerData fake_ctid;
    ExecClearTuple(slot);

    char buf[4];

    h->is.read(buf, sizeof buf);

    elog(DEBUG5, "read next %d bytes", h->is.gcount());

    if (h->is.gcount() < sizeof buf) {
      return false;
    }

    scan->curreof += 4;

    int sz = (1 << 24) * buf[0] + (1 << 16) * buf[1] + (1 << 8) * buf[2] + buf[3];

    char bufbody[sz];

    h->is.read(bufbody, sizeof bufbody);
    elog(DEBUG5, "read next %d bytes", h->is.gcount());

    if (h->is.gcount() < sz) {
      return false;
    }

    scan->curreof += sz;

    auto mt_bind = create_memtuple_binding(
        RelationGetDescr(relation), RelationGetNumberOfAttributes(relation));

    memtuple_deform((MemTuple)bufbody, mt_bind, slot->tts_values,
                    slot->tts_isnull);
    slot->tts_tid = fake_ctid;
    ExecStoreVirtualTuple(slot);

    pfree(mt_bind);
    scan->currtup++;
    return true;
  }
  if (slot) {
    ExecClearTuple(slot);
  }

  return false;
}


void yeneid_scan_cleanup_internal(YeneidScanDesc scan) {
  delete (YeneidMetadataState*)scan->YeneidMetadataState;
}

void yeneid_scan_init(YeneidScanDesc scan) {
 scan-> YeneidMetadataState = new YeneidMetadataState(RelationGetRelid(scan->rs_base.rs_rd));
}