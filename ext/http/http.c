/* vim:set noexpandtab nosmarttab softtabstop=0 tabstop=8 shiftwidth=8: */
#include "lhttpd.h"

static int http_accept_cb(int fd)
{
}

LH_API int lh_http_init()
{
	lh_reg("http_accept", &lh_http_accept_cb);
}
