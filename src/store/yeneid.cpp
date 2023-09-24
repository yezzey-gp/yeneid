#include "yeneid.h"

/*
* Utility code 
*/

MemTuple
yeneid_form_memtuple(TupleTableSlot *slot, MemTupleBinding *mt_bind)
{
	MemTuple		result;
	MemoryContext	oldContext;

	/*
	 * In case of a non virtal tuple, make certain that the slot's values are
	 * populated, for example during a CTAS.
	 */
	if (!TTS_IS_VIRTUAL(slot))
		slot_getsomeattrs(slot, slot->tts_tupleDescriptor->natts);

	oldContext = MemoryContextSwitchTo(slot->tts_mcxt);
	result = memtuple_form(mt_bind,
						   slot->tts_values,
						   slot->tts_isnull);
	MemoryContextSwitchTo(oldContext);

	return result;
}


/*
 * Retrieve the state information for a relation.
 * It is required that the state has been created before hand.
 */
YeneidMetadataState *
get_yeneid_metadata(const Oid relationOid)
{
    static std::unordered_map<Oid, YeneidMetadataState> cache_;
    if (!cache_.count(relationOid)) {
        cache_[relationOid] = YeneidMetadataState(relationOid);
    }


	return &cache_[relationOid];
}


void
yeneid_tuple_insert_internal(Relation relation, TupleTableSlot *slot,
					   CommandId cid, int options, BulkInsertState bistate)
{
    /* cahce binding */
    auto mt_bind = create_memtuple_binding(RelationGetDescr(relation), RelationGetNumberOfAttributes(relation));
    auto memtup = yeneid_form_memtuple(slot, mt_bind);

    auto h = get_yeneid_metadata(RelationGetRelid(relation));

    h->tuples.push_back(memtup);


	pfree(memtup);
    pfree(mt_bind);
}


/* ----------------
 *		yeneid_getnextslot - retrieve next tuple in scan
 * ----------------
 */
bool
yeneid_scan_getnextslot_internal(YeneidScanDesc scan, ScanDirection direction, TupleTableSlot *slot)
{
    auto h = get_yeneid_metadata(RelationGetRelid(rs_base.rs_rd));

	if (scan->currtup + 1 <= h->tuples.size())
	{
        ItemPointerData fake_ctid;
		ExecClearTuple(slot);
		memtuple_deform(h->tuples[scan->currtup], executorReadBlock->mt_bind, slot->tts_values, slot->tts_isnull);
		slot->tts_tid = fake_ctid;
		ExecStoreVirtualTuple(slot);

        scan->currtup++;
		return true;
	}
	else
	{
		if (slot)
			ExecClearTuple(slot);

		return false;
	}
}
