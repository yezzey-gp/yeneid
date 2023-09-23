MODULES = yeneid

EXTENSION = yeneid
DATA = yeneid--1.0.sql
PGFILEDESC = "yeneid - table AM for Yezzey"

REGRESS = yeneid

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

