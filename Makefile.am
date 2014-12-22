ACLOCAL_AMFLAGS = -I m4
AM_LDFLAGS = @BOOST_LDFLAGS@ @LUA_LIB@ @COVERAGE_LDFLAGS@
AM_CPPFLAGS = -Ivalhalla -Ivalhalla/proto @BOOST_CPPFLAGS@
AM_CXXFLAGS = -Ivalhalla -Ivalhalla/proto @COVERAGE_CXXFLAGS@ @LUA_INCLUDE@
VALHALLA_LDFLAGS = @VALHALLA_MIDGARD_LDFLAGS@ @VALHALLA_MIDGARD_LIB@ @VALHALLA_BALDR_LDFLAGS@ @VALHALLA_BALDR_LIB@
VALHALLA_CPPFLAGS = @VALHALLA_MIDGARD_CPPFLAGS@ @VALHALLA_BALDR_CPPFLAGS@
LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

# conditional test coverage
if ENABLE_COVERAGE
.PHONY: clean-coverage
clean-coverage:
	-find -name '*.gcda' -exec rm -rf {} \;
	-$(LCOV) --directory $(top_builddir) -z
	-rm -rf coverage.info coverage/

.PHONY: coverage-report
coverage-report: clean-coverage
	-$(MAKE) $(AM_MAKEFLAGS) -k check
	$(MAKE) $(AM_MAKEFLAGS) coverage/index.html

coverage.info:
	$(LCOV) --directory $(top_builddir) --base-directory $(top_builddir) --no-external --capture --output-file $@ --no-checksum --compat-libtool

coverage/index.html: coverage.info
	$(GENHTML) --prefix $(top_builddir) --output-directory $(@D) --title "Test Coverage" --legend --show-details $<


.PHONY: clean-gcno
clean-gcno:
	-find -name '*.gcno' -exec rm -rf {} \;

clean-local: clean-coverage clean-gcno
endif

# proto generation etc
PROTO_FILES = proto/osmformat.proto proto/fileformat.proto
BUILT_SOURCES = $(patsubst %.proto,src/%.pb.cc,$(PROTO_FILES))
CLEANFILES = $(patsubst %.proto,valhalla/%.pb.h,$(PROTO_FILES)) $(patsubst %.proto,src/%.pb.cc,$(PROTO_FILES))
nodist_libvalhalla_mjolnir_la_SOURCES = $(patsubst %.proto,src/%.pb.cc,$(PROTO_FILES))
src/proto/%.pb.cc: proto/%.proto
	@echo " PROTOC $<"; mkdir -p src/proto valhalla/proto; @PROTOC_BIN@ -Iproto --cpp_out=valhalla/proto $< && mv valhalla/proto/$(@F) src/proto

# lib valhalla compilation etc
lib_LTLIBRARIES = libvalhalla_mjolnir.la
nobase_include_HEADERS = \
	valhalla/mjolnir/directededgebuilder.h \
	valhalla/mjolnir/graphtilebuilder.h \
	valhalla/mjolnir/graphtileheaderbuilder.h \
	valhalla/mjolnir/nodeinfobuilder.h \
	valhalla/mjolnir/edgeinfobuilder.h \
	valhalla/mjolnir/exitsignbuilder.h 
libvalhalla_mjolnir_la_SOURCES = \
	src/mjolnir/directededgebuilder.cc \
	src/mjolnir/graphtilebuilder.cc \
	src/mjolnir/graphtileheaderbuilder.cc \
	src/mjolnir/nodeinfobuilder.cc \
	src/mjolnir/edgeinfobuilder.cc \
	src/mjolnir/exitsignbuilder.cc \
	src/proto/fileformat.pb.cc \
	src/proto/osmformat.pb.cc 
libvalhalla_mjolnir_la_CPPFLAGS = $(DEPS_CFLAGS) $(VALHALLA_CPPFLAGS) @BOOST_CPPFLAGS@
libvalhalla_mjolnir_la_LIBADD = $(DEPS_LIBS) $(VALHALLA_LDFLAGS) @BOOST_LDFLAGS@ @BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_FILESYSTEM_LIB@ @PROTOC_LIBS@

#distributed executables
bin_PROGRAMS = pbfgraphbuilder
pbfgraphbuilder_SOURCES = \
	src/mjolnir/pbfgraphbuilder/pbfgraphbuilder.cc \
	src/mjolnir/pbfgraphbuilder/graphbuilder.cc \
	src/mjolnir/pbfgraphbuilder/luatagtransform.cc \
	src/mjolnir/pbfgraphbuilder/osmnode.cc
pbfgraphbuilder_CPPFLAGS = $(DEPS_CFLAGS) $(VALHALLA_CPPFLAGS) @BOOST_CPPFLAGS@
pbfgraphbuilder_LDADD = $(DEPS_LIBS) $(VALHALLA_LDFLAGS) @BOOST_LDFLAGS@ @BOOST_PROGRAM_OPTIONS_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_SYSTEM_LIB@ @PROTOC_LIBS@ -lz libvalhalla_mjolnir.la

# tests
check_PROGRAMS = \
	test/edgeinfobuilder
test_edgeinfobuilder_SOURCES = test/edgeinfobuilder.cc test/test.cc
test_edgeinfobuilder_CPPFLAGS = $(DEPS_CFLAGS) $(VALHALLA_CPPFLAGS) @BOOST_CPPFLAGS@
test_edgeinfobuilder_LDADD = $(DEPS_LIBS) $(VALHALLA_LDFLAGS) @BOOST_LDFLAGS@ libvalhalla_mjolnir.la

TESTS = $(check_PROGRAMS)
TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = sh

test: check