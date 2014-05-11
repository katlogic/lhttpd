/* vim:set noexpandtab nosmarttab softtabstop=0 tabstop=8 shiftwidth=8: */

/* Types. */
typedef int fd_t;
enum {
	LH_PIPE_READ,
	LH_PIPE_WRITE,
	LH_PIPE_QUEUE,
	LH_PIPE_MAX
};
struct lh_pipe {
	int		(*cb)(fd_t fd, void *data);
	union {
		void		*data;
		uint8_t 	meta[4];
	}
	struct lh_pipe 	*next;
};

/* Some utility macros. */
#define LH_ZERO(s) memset(&(s), 0, sizeof(s))
#define LH_API


/* API for event provider. */
LH_API const char *lh_ev_ident;
LH_API int lh_ev_init();
LH_API void lh_ev_mod(fd_t fd, int events);
LH_API void lh_ev_add(fd_t fd, int events);
LH_API void lh_ev_del(fd_t fd);

/* FD operations. */
LH_API int lh_setnb(fd_t fd);

/* Pipe operations. */
LH_API void lh_reg_num(char *sym, int n);
LH_API void lh_reg(char *sym, void *v);
LH_API lh_pipe *lh_pipe_lookup(int ev, uint8_t pn, int i);
LH_API int lh_pipe_del(int ev, uint8_t pn, int i);
LH_API lh_pipe *lh_pipe_add(int ev, uint8_t pn, int *i);
LH_API int lh_pipe_get(int ev, uint8_t pn, int i, lh_pipe ***rp);

/* Globals. */
LH_API struct lh_pipe *lh_pipes[LH_PIPE_MAX][256];	/* Pipe definitions. */
LH_API uint8_t lh_fds[LH_PIPE_MAX][LH_MAXFD];		/* Pipelines for events.  */
LH_API lua_State *lh_L;					/* Lua state. */



