lib_LTLIBRARIES = libvtls-gnutls.la
libvtls_gnutls_la_SOURCES = vtls.c vtls.h timeval.c timeval.h select.c select.h \
 inet_pton.c inet_pton.h common.c common.h gnutls.c gnutls.h

libvtls_gnutls_la_CPPFLAGS = -I$(top_srcdir)/include -I$(srcdir)
libvtls_gnutls_la_LDFLAGS = -version-info $(LIBVTLS_SO_VERSION) -lgcrypt
