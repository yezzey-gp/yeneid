
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

#include <fstream>

struct YeneidMetadataState {
  Oid relationOid;
  int fd{-1};
  int logicaleof;

  SMgrRelation smgr;


  int npagecnt;


  YeneidMetadataState() {}
  YeneidMetadataState(Oid relationOid, SMgrRelation smgr) : relationOid(relationOid), smgr(smgr) {
    npagecnt = smgrnblocks(smgr, MAIN_FORKNUM);
  }

protected:
};

#endif

EXTERNC bool yeneid_scan_getnextslot_internal(YeneidScanDesc scan,
                                              ScanDirection direction,
                                              TupleTableSlot *slot);

EXTERNC void yeneid_tuple_insert_internal(Relation relation,
                                          TupleTableSlot *slot, CommandId cid,
                                          int options, BulkInsertState bistate);


EXTERNC void yeneid_scan_cleanup_internal(YeneidScanDesc scan);

EXTERNC void yeneid_scan_init(YeneidScanDesc scan);

EXTERNC void
yeneid_relation_set_new_relfilenode_internal(Relation rel,
								 const RelFileNode *newrnode,
								 char persistence,
								 TransactionId *freezeXid,
								 MultiXactId *minmulti);

#endif /* YENEID_STORAGE_H */
