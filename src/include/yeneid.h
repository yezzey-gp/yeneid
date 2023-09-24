#ifndef YENEID_YENEID_H
#define YENEID_YENEID_H

#include "pg.h"

#include "externc.h"


#include "miscadmin.h"

#include "access/tableam.h"
#include "access/heapam.h"
#include "access/amapi.h"
#include "catalog/index.h"
#include "commands/vacuum.h"
#include "executor/tuptable.h"


/* Base structures for scans */
typedef struct YeneidScanDescData
{
	TableScanDescData rs_base;	/* AM independent part of the descriptor */
	
	/* Add more fields here as needed by the AM. */

	int currtup;
} YeneidScanDescData;
typedef struct YeneidScanDescData *YeneidScanDesc;


#ifdef __cplusplus
/*
 */
#include <vector>
#include <unordered_map>
#include <map>
struct YeneidMetadataState
{
	const Oid relationOid;
    std::vector<MemTuple> tuples;

    YeneidMetadataState(const Oid relationOid) : relationOid(relationOid) {
    }
};


#endif


EXTERNC bool yeneid_scan_getnextslot_internal(
    YeneidScanDesc scan,
    ScanDirection direction,
    TupleTableSlot *slot);

EXTERNC void yeneid_tuple_insert_internal(
    Relation relation,
    TupleTableSlot *slot,
	CommandId cid, int options, BulkInsertState bistate);


#endif /* YENEID_YENEID_H */
