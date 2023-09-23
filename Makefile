MODULES = yeneid

EXTENSION = yeneid
DATA = yeneid--1.0.sql
PGFILEDESC = "yeneid - table AM for Yezzey"

REGRESS = yeneid

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = gpcontrib/yeneid
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

