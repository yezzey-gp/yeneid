
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

  std::ifstream is;

  std::ofstream os;

  YeneidMetadataState() {}
  YeneidMetadataState(Oid relationOid) : relationOid(relationOid), is("yeneid_table"), os("yeneid_table",  std::ios::binary |  std::fstream::app) {
    is.seekg(0, std::ios::beg);
    os.seekp(0, std::ios::end);
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

#endif /* YENEID_STORAGE_H */
