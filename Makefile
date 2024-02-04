
override CFLAGS = -Wall -Wmissing-prototypes -Wpointer-arith -Wendif-labels -Wmissing-format-attribute -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -fno-aggressive-loop-optimizations -Wno-unused-but-set-variable -Wno-address -Wno-format-truncation -Wno-stringop-truncation -g -ggdb -std=gnu99 -Werror=uninitialized -Werror=implicit-function-declaration -DGPBUILD

# -Werror=implicit-fallthrough=3
override CXX = g++-8

COMMON_LINK_OPTIONS = -lstdc++ -lxml2 -lpthread -lcrypto -lcurl -lz -lgpgme -lstdc++fs

COMMON_CPP_FLAGS = -std=c++11 -fPIC -I/usr/include/libxml2 -I/usr/local/opt/openssl/include -DENABLE_NLS -DHAVE_CRYPTO

override CPPFLAGS = -fPIC -lstdc++fs -lstdc++ -lxml2 -lpthread -lcrypto -lcurl -lgpgme -lz -g3 -ggdb -Wall -Wpointer-arith -Wendif-labels -Wmissing-format-attribute -Wformat-security -fno-strict-aliasing -fwrapv -fno-aggressive-loop-optimizations -Wno-unused-but-set-variable -Wno-address -Werror=format-security -Wno-format-truncation -Wno-stringop-truncation -g -ggdb -std=c++11 -fPIC -I/usr/include/libxml2 -I/usr/local/opt/openssl/include -Iinclude -Ilib -I../../src/interfaces/libpq -I../../src/interfaces/libpq/postgresql/server/utils -g -I. -I. -I../../src/include -D_GNU_SOURCE -I/usr/include/libxml2  -DGPBUILD



SHLIB_LINK += $(COMMON_LINK_OPTIONS)
PG_CPPFLAGS += $(COMMON_CPP_FLAGS) -I./include -Iinclude -Ilib -I$(libpq_srcdir) -I$(libpq_srcdir)/postgresql/server/utils


MODULE_big = yeneid

OBJS = \
	$(WIN32RES) \
	src/storage.o \
	src/virt_tablespace.o \
	yeneid.o

EXTENSION = yeneid
DATA = yeneid--1.0.sql
PGFILEDESC = "yeneid - external storage tables offloading extension"

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

cleanall:
	@-$(MAKE) clean # incase PGXS not included
	rm -f *.o *.so *.a
	rm -f *.gcov src/*.gcov src/*.gcda src/*.gcno
	rm -f src/*.o src/*.d


apply_fmt:
	clang-format -i ./src/*.cpp ./include/*.h ./*.c

format:
	@-[ -n "`command -v dos2unix`" ] && dos2unix -k -q src/*.cpp bin/gpcheckcloud/*.cpp test/*.cpp include/*.h
	@-[ -n "`command -v clang-format`" ] && clang-format -style="{BasedOnStyle: Google, IndentWidth: 4, ColumnLimit: 100, AllowShortFunctionsOnASingleLine: None}" -i src/*.cpp bin/gpcheckcloud/*.cpp test/*.cpp include/*.h
