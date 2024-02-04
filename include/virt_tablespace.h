#ifndef YENEID_VIRTUAL_TABLESPACE
#define YENEID_VIRTUAL_TABLESPACE

#include "pg.h"

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void YeneidATExecSetTableSpace(Relation aorel, Oid reloid,
                                       Oid desttablespace_oid);


EXTERNC void YeneidDefineOffloadPolicy(Oid reloid);

#endif /* YENEID_VIRTUAL_TABLESPACE */