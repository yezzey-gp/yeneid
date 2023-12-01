#ifndef YENEID_SCAN_H
#define YENEID_SCAN_H

#include "pg.h"

#include "externc.h"

#include "miscadmin.h"

#include "access/amapi.h"
#include "access/heapam.h"
#include "access/tableam.h"
#include "catalog/index.h"
#include "commands/vacuum.h"
#include "executor/tuptable.h"

/* Base structures for scans */
typedef struct YeneidScanDescData {
  TableScanDescData rs_base; /* AM independent part of the descriptor */

  /* Add more fields here as needed by the AM. */

  int current_block;
  char * page_ptr;

  char buf[BLCKSZ];

  // field of type YeneidMetadataState*
  void * YeneidMetadataState;
} YeneidScanDescData;
typedef struct YeneidScanDescData *YeneidScanDesc;

#endif /* YENEID_SCAN_H */