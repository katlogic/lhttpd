/* vim:set noexpandtab nosmarttab softtabstop=0 tabstop=8 shiftwidth=8: */
#include "lhttpd.h"

static FD_SET rfds;
static FD_SET wfds;

static int fd_max = 0;

LH_API const char *lh_ev_ident = "select";

LH_API int lh_ev_init()
{
	return FD_SETSIZE;
}

LH_API void lh_ev_mod(fd_t fd, int events)
{
	if (events & LH_EV_READ)
		FD_SET(fd, &rfds);
	else
		FD_CLR(fd, &rfds);

	if (events & LH_EV_WRITE)
		FD_SET(fd, &wfds);
	else
		FD_CLR(fd, &wfds);
}

LH_API void lh_ev_add(fd_t fd, int events)
{
	ev_mod(fd, events);
}

LH_API void lh_ev_del(fd_t fd)
{
	ev_mod(fd, 0);
}

