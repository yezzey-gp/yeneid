
#ifndef YENEID_STORAGE_H
#define YENEID_STORAGE_H

#include "externc.h"
#include "pg.h"
#include "yscan.h"

#ifdef __cplusplus
/*
 */
#include <map>
#include <unordered_map>
#include <vector>
struct YeneidMetadataState {
  Oid relationOid;
  std::vector<MemTuple> tuples;

  YeneidMetadataState() {}
  YeneidMetadataState(Oid relationOid) : relationOid(relationOid) {}
};

#endif

EXTERNC bool yeneid_scan_getnextslot_internal(YeneidScanDesc scan,
                                              ScanDirection direction,
                                              TupleTableSlot *slot);

EXTERNC void yeneid_tuple_insert_internal(Relation relation,
                                          TupleTableSlot *slot, CommandId cid,
                                          int options, BulkInsertState bistate);

#endif /* YENEID_STORAGE_H */
