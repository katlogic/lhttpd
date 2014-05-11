/* vim:set noexpandtab nosmarttab softtabstop=0 tabstop=8 shiftwidth=8: */

/**************************************
 * PUBLIC 
 **************************************/

/* Propagate errno to Lua. */
LH_API int lh_errno(lua_State *L)
{
	int err = errno;
	lua_pushnil(L);
	lua_pushstring(L, strerror(err));
	lua_pushinteger(L, err);
	return 3;
}

/* Set fd as nonblocking. */
LH_API int lh_setnb(fd_t fd)
{
	fd_t o = fcntl(fd, F_GETFL);
	if (o < 0)
		return -1;
	return fcntl(fd, F_SETFL, o | O_NONBLOCK);
}

/* Load meta bytes from given fd/event. */
LH_API const uint8_t *lh_pipe_getmeta(int srcfd, int srcev)
{
	return lh_pipes[lh_fds[srcev][srcfd]].meta;
}

/* Reconfigure pipes in dstfd according to template from srcfd/srcev. */
LH_API void lh_pipe_reconf(int dstfd, int srcfd, int srcev)
{
	const uint8_t *newpipes = lh_pipe_getmeta(srcfd, srcev);
	for (i = 0; i < LH_PIPE_MAX; i++)
		lh_fds[i][newfd] = newpipes[i];
}

/* Run pipe callback. */
LH_API int lh_pipe_run(int ev, int fd, void *data)
{
	lh_pipe *p;
	if (!lh_fds[ev][fd])
		return 0;
	p = lh_pipes[lh_fds[ev][fd]];
	if (!p)
		return 0;
	return p->cb(fd, data);
}

/* Find given pipe i (or return last index if not found). */
LH_API int lh_pipe_get(int ev, uint8_t pn, int i, lh_pipe ***rp)
{
	int ri = 0;
	lh_pipe *p, **pp = &lh_pipe[ev][pn];
	for (p = lh_pipe[pn]; p && pi; pp = &p->next, p = p->next) {
		pi--;
		ri++;
	}
	*rp = pp;
	return ri;
}

/* Create pipe cell given event, pipe number and chain index. */
LH_API lh_pipe *lh_pipe_add(int ev, uint8_t pn, int *i)
{
	lh_pipe *p, **pp;
	int ret = lh_pipe_get(ev, pn, *i, &pp);
	*i = ret;
	p = malloc(sizeof(*p));
	p->next = *pp;
	*pp = p;
	return p;
}

/* Delete pipe cell. */
LH_API int lh_pipe_del(int ev, uint8_t pn, int i)
{
	lh_pipe *p, **pp;
	if (lh_pipe_get(ev, pn, *i, &pp) != i)
		return 0;
	if (!*pp)
		return 0;
	p = *pp;
	*pp = p->next;
	free(p);
	return 1;
}

/* Find given pipe. */
LH_API lh_pipe *lh_pipe_lookup(int ev, uint8_t pn, int i)
{
	lh_pipe *p, **pp;
	if (lh_pipe_get(ev, pn, *i, &pp) != i)
		return NULL;
	if (!*pp)
		return NULL;
	return p;
}

/* Set lh.sym[sym] = v. */
LH_API void lh_reg(char *sym, void *v)
{
	lua_State *L = lh_L;
	lua_getglobal(L, "lh");
	lua_getfield(L, "sym", -1);
	lua_pushlightuserdata(L, v);
	lua_setfield(L, sym, -2);
	lua_pop(L, 1);
}

/* Set lh.sym[sym] = n. */
LH_API void lh_reg_num(char *sym, int n)
{
	lua_State *L = lh_L;
	lua_getglobal(L, "lh");
	lua_getfield(L, "sym", -1);
	lua_pushinteger(L, n);
	lua_setfield(L, sym, -2);
	lua_pop(L, 1);
}

/**************************************
 * PRIVATE
 **************************************/
/* lh.tcpserver(ip, port, queue) */
static int lh_tcpserver(lua_State *L)
{
	sockaddr_in sin;
	fd_t fd;

	LH_ZERO(sin);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(lua_tounsigned(L, 1));
	sin.sin_port = htons(lua_tounsigned(L, 2));

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
		return lh_errno(L);
	if (bind(fd, (struct sockaddr*)&sin, sizeof sin))
		return lh_errno(L);
	if (listen(fd, lua_tounsigned(L, 3)))
		return lh_errno(L);
	lh_setnb(fd);
	lua_pushunsigned(L, fd);
	return 1;
}

/* lh.pipeadd(ev, pipeno, index) */
static int lh_pipeadd(lua_State *L)
{
	int i = lua_tointeger(L, 3)
	lh_pipe_add(lua_tointeger(L, 1), lua_tointeger(L, 2), &i);
	lua_pushinteger(L, i);
	return 1;
}

/* lh.pipedel(ev, pipeno,index) */
static int lh_pipedel(lua_State *L)
{
	int i = lua_tointeger(L, 3)
	lua_pushboolean(L, 1);
	return lh_pipe_del(lua_tointeger(L, 1), lua_tointeger(L, 2), i);
}

/* lh.pipeset(pipeno,index,t,cb,data) */
static int lh_pipeset(lua_State *L)
{
	int i = lua_tointeger(L, 3)
	lua_pushboolean(L, 1);
	lh_pipe *p = lh_pipe_lookup(lua_tointeger(L, 1), lua_tointeger(L, 2), i);
	if (!p)
		return 0;
	p->cb = lua_touserdata(L, 1);
	p->data = lua_touserdata(L, 2);
	return 1;
}

/* Accept TCP clients.
 *
 * LH_PIPE_READ - this callback
 * LH_PIPE_WRITE - holds new client configuration template,
 * 		   cb is unused.
 * LH_PIPE_QUEUE - holds callback to be called with newfd
 */
static int lh_cb_tcp_accept(fd_t fd, void *data)
{
	struct {
		struct sockaddr sin;
		int newfd;
		int err;
		int fd;
	} info;
	socklen_t sinlen = sizeof(info.sin);
	int i, nfd;

	/* Accept the client. */
	info.fd = fd;
	nfd = info.newfd = accept(fd, (struct sockaddr*)&info.sin, &sinlen);
	if (nfd >= 0) { /* Reconfig if everything is ok. */
		lh_setnb(nfd);
		lh_ev_add(nfd, 0); /* Reads are enabled by notify callback. */
		lh_pipe_reconf(newfd, fd, LH_PIPE_WRITE);
	} else {
		info.err = errno;
	}
	lh_pipe_run(LH_PIPE_QUEUE, fd, &info); /* Notify. */
}


/* Lua API functions. */
static const luaL_Reg lhttpd_api[] = {
	{ "tcpserver", lh_tcpserver },
	{ "pipeadd", lh_pipeadd },
	{ "pipedel", lh_pipedel },
	{ "pipeset", lh_pipeset },
	{ NULL, NULL }
};

static int lhttpd_init(lua_State *L)
{
	luaL_newlib(L, lhttpd_api);
	return 1;
}

int main(int argc, char **argv)
{
	lh_L = luaL_newstate();
	luaL_requiref(lh_L, "lh", lhttpd_init, 1);
	lua_newtable(L); /* Symbol table. */
	lua_setfield(L, -2, "sym");
	lh_reg("read_tcp_accept", &lh_cb_tcp_accept);
	luaL_require(lh_L, "lhttpd");
	return 1;
}

