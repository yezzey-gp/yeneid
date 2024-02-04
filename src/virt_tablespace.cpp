#include "virt_tablespace.h"
#include "utils/builtins.h"


/*
 * Execute ALTER TABLE SET TABLESPACE for cases where there is no tuple
 * rewriting to be done, so we just want to copy the data as fast as possible.
 */
void YeneidATExecSetTableSpace(Relation aorel, Oid reloid,
                               Oid desttablespace_oid) {
  /*
   * Need lock here in case we are recursing to toast table or index
   */

  /*
   * We cannot support moving mapped relations into different tablespaces.
   * (In particular this eliminates all shared catalogs.)
   */
  if (RelationIsMapped(aorel))
    ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                    errmsg("cannot move system relation \"%s\"",
                           RelationGetRelationName(aorel))));

  /*
   * Don't allow moving temp tables of other backends ... their local buffer
   * manager is not going to cope.
   */
  if (RELATION_IS_OTHER_TEMP(aorel))
    ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                    errmsg("cannot move temporary tables of other sessions")));

  /* Fetch the list of indexes on toast relation if necessary */
  Assert(!OidIsValid(aorel->rd_rel->reltoastrelid));

  // /* Get the bitmap sub objects */
  // if (RelationIsBitmapIndex(rel))
  // 	GetBitmapIndexAuxOids(rel, &relbmrelid, &relbmidxid);

  /* Get a modifiable copy of the relation's pg_class row */
  auto pg_class = heap_open(RelationRelationId, RowExclusiveLock);

  /* Get a modifiable copy of the relation's yezzey distrib row */
  auto yeneid_distrib = heap_open(YezzeyDistribRelationId, RowExclusiveLock);

  auto tuple = SearchSysCacheCopy1(RELOID, ObjectIdGetDatum(reloid));
  if (!HeapTupleIsValid(tuple))
    elog(ERROR, "cache lookup failed for relation %u", reloid);
  auto rd_rel = (Form_pg_class)GETSTRUCT(tuple);

  /*
   * Since we copy the file directly without looking at the shared buffers,
   * we'd better first flush out any pages of the source relation that are
   * in shared buffers.  We assume no new changes will be made while we are
   * holding exclusive lock on the rel.
   */
  FlushRelationBuffers(aorel);

  /*
   * Relfilenodes are not unique in databases across tablespaces, so we need
   * to allocate a new one in the new tablespace.
   */
  /* Open old and new relation */
  /*
   * Create and copy all forks of the relation, and schedule unlinking of
   * old physical files.
   *
   * NOTE: any conflict in relfilenode value will be caught in
   * RelationCreateStorage().
   */
  /* data already copied */
  /*
   * Append-only tables now include init forks for unlogged tables, so we copy
   * over all forks. AO tables, so far, do not have visimap or fsm forks.
   */

  /* drop old relation, and close new one */
//   RelationDropStoragePure(aorel);

  RelationDropStorage(aorel);

  /* update the pg_class row */
  if (desttablespace_oid != HEAPYTABLESPACE_OID) {
    rd_rel->relfilenode = GetNewRelFileNode(desttablespace_oid, NULL,
                                            aorel->rd_rel->relpersistence);
  }
  rd_rel->reltablespace = desttablespace_oid;

#if GP_VERSION_NUM < 70000
  simple_heap_update(pg_class, &tuple->t_self, tuple);
  CatalogUpdateIndexes(pg_class, tuple);
#else
  CatalogTupleUpdate(pg_class, &tuple->t_self, tuple);
#endif

  /*
    * Open and lock the gp_fastsequence catalog table.
    */
  auto tupleDesc = RelationGetDescr(yeneid_distrib);

  auto values = (Datum*) palloc0(sizeof(Datum) * tupleDesc->natts);
  auto nulls = (bool*)palloc0(sizeof(bool) * tupleDesc->natts);

  int16 arr[2] = {0, 1};

  values[Anum_yezzey_distrib_reloid - 1] = ObjectIdGetDatum(reloid);
  values[Anum_yezzey_distrib_distkey - 1] = PointerGetDatum(
      buildint2vector(arr, 2)
  );

  auto ytuple = heaptuple_form_to(tupleDesc, values, nulls, NULL, NULL);

  CatalogTupleInsert(yeneid_distrib, ytuple);

  RelationCloseSmgr(aorel);

  aorel->rd_node.relNode = rd_rel->relfilenode ;

  aorel->rd_node.spcNode = desttablespace_oid;

  RelationOpenSmgr(aorel);


//   RelationCreateStorage(aorel->rd_node, rd_rel->relpersistence, SMGR_MD);

  InvokeObjectPostAlterHook(RelationRelationId, RelationGetRelid(aorel), 0);

  heap_freetuple(tuple);

  heap_close(pg_class, RowExclusiveLock);

  heap_close(yeneid_distrib, RowExclusiveLock);

  // Yeneid: do we need this?
  // /* MPP-6929: metadata tracking */
  // if ((Gp_role == GP_ROLE_DISPATCH) && MetaTrackValidKindNsp(rel->rd_rel))
  // 	MetaTrackUpdObject(RelationRelationId,
  // 					   RelationGetRelid(rel),
  // 					   GetUserId(),
  // 					   "ALTER", "SET TABLESPACE");

  /* Make sure the reltablespace change is visible */
  CommandCounterIncrement();

  /* Yeneid: do we need to move indexes? */
  // /*
  //  * MPP-7996 - bitmap index subobjects w/Alter Table Set tablespace
  //  */
  // if (OidIsValid(relbmrelid))
  // {
  // 	Assert(!relaosegrelid);
  // 	ATExecSetTableSpace(relbmrelid, newTableSpace, lockmode);
  // }
  // if (OidIsValid(relbmidxid))
  // 	ATExecSetTableSpace(relbmidxid, newTableSpace, lockmode);

  /* Clean up */
}


void YeneidDefineOffloadPolicy(Oid reloid) {

  /* Open relation
   */
  auto aorel = relation_open(reloid, AccessExclusiveLock);

  /* change relation tablespace */
  (void)YeneidATExecSetTableSpace(aorel, reloid, HEAPYTABLESPACE_OID);

  relation_close(aorel, AccessExclusiveLock);
}