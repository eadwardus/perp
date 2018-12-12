#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

/* buf macros */
#define buf_zero(b,n) buf_fill((b),(n),(0))
#define buf_WIPE(b,n) buf_fill((b),(n),(0))

/* cdb macros */
#define cdb_pbase(C)           (0)
#define cdb_rbase(C)           (256 * 8)
#define cdb_fd(C)              (C)->fd
#define cdb_map(C)             (C)->map
#define cdb_mapsize(C)         (C)->map_size
#define cdb_dlen(C)            ((C)->dlen)
#define cdb_dpos(C)            ((C)->rpos + 8 + (C)->klen)
#define cdb_klen(C)            ((C)->klen)
#define cdb_kpos(C)            ((C)->rpos + 8)
#define cdb_rpos(C)            ((C)->rpos)
#define CDB_NTABS              256
#define CDB_HASHINIT           5381
#define cdb_hash(key, klen)    cdb_hashpart(CDB_HASHINIT, (key), (klen))
#define cdb_NTAB(hash)         ((hash) & 255)
#define cdb_SLOT(hash, tslots) ((hash>>8) % (tslots))

/* cdbmk macros */
#define CDBMK_NLISTS  4
#define CDBMK_BUFSIZE 8192
#define cdbmk_NLIST(ntab) ((ntab) < 128) ? (((ntab) < 64)  ? 0 : 1) : (((ntab) < 192) ? 2 : 3)
#define CDBMK_NRECS 500

/* cstr macros */
#define cstr_eq(s1, s2)     (cstr_cmp((s1),(s2))==0)
#define cstr_eqi(s1, s2)    (cstr_cmpi((s1),(s2))==0)
#define cstr_vcat(to, ...)  cstr_vcat_(to, __VA_ARGS__, NULL)
#define cstr_vcopy(to, ...) cstr_vcopy_(to, __VA_ARGS__, NULL)
#define cstr_vlen(...)      cstr_vlen_(__VA_ARGS__, NULL)

/* devout macros */
#define devout(fd, ...) devout_((fd), __VA_ARGS__, NULL)

/* die macros */
#define die(e)  _exit((int)(e))

/* domsock macros */
#define domsock_listen(s,q) listen((s),(q))

/* dynbuf macros */
#define DYNBUF_INITSIZE    1
#define dynbuf_INIT()      { NULL, 0, 0 }
#define dynbuf_CLEAR(d)    ((d)->p = 0)
#define dynbuf_BUF(d)      ((d)->buf)
#define dynbuf_LEN(d)      ((d)->p)
#define dynbuf_putc(d,c)   dynbuf_putbuf((d),&(c),1)
#define dynbuf_putNUL(d,c) dynbuf_putb((d),'\0')

/* dynstr macros */
#define DYNSTR_INITSIZE     1
#define dynstr_INIT()       { NULL, 0, 0 }
#define dynstr_CLEAR(S)     { (S)->n = 0; if((S)->k) (S)->s[0] = '\0'; }
#define dynstr_STR(S)       ((S)->s)
#define dynstr_LEN(S)       ((S)->n)
#define dynstr_vputs(S,...) dynstr_vputs_((S), __VA_ARGS__, NULL)

/* dynstuf macros */
#define dynstuf_INIT()     { NULL, 0, 0 }
#define dynstuf_ITEMS(S)   ((S)->items)
#define dynstuf_ISEMPTY(S) ((S)->items == 0)
#define dynstuf_SLOTS(S)   ((S)->slots)
#define dynstuf_STUF(S)    ((S)->stuf)

/* execvx macros */
#ifndef EXECVX_PATH
#define EXECVX_PATH "/bin:/usr/bin"
#endif

/* hdb macros */
#define HDB_IDLEN              16
#define HDB32_IDENT            "hdb32/1.0\0\0\0\0\0\0\0"
#define HDB_NTABS              8
#define HDB_CBASE              (HDB_IDLEN + 8 + (HDB_NTABS * 8))
#define HDB_U24MAX             ((1 << 24) - 1)
#define hdb_cbase(H)           (H)->cbase
#define hdb_clen(H)            (hdb_rbase((H)) - hdb_cbase((H)))
#define hdb_hbase(H)           (H)->hbase
#define hdb_rbase(H)           (H)->rbase
#define hdb_fd(H)              (H)->fd
#define hdb_map(H)             (H)->map
#define hdb_mapsize(H)         (H)->map_size
#define hdb_dlen(H)            ((H)->dlen)
#define hdb_dpos(H)            ((H)->rpos + 6 + (H)->klen)
#define hdb_klen(H)            ((H)->klen)
#define hdb_kpos(H)            ((H)->rpos + 6)
#define hdb_rpos(H)            ((H)->rpos)
#define HDB_HASHINIT           0x0
#define HDB_HWORD              16
#define HDB_SHIFT              3
#define hdb_NTAB(hash)         ((hash) & (HDB_NTABS - 1))
#define hdb_XMIX(hash)         (((hash) >> (HDB_HWORD - HDB_SHIFT)) ^ (hash))
#define hdb_SLOT(hash, tslots) ((hdb_XMIX((hash)) >> HDB_SHIFT) % (tslots))
#define hdb_hash(key, klen)    hdb_hashpart(HDB_HASHINIT, (key), (klen))

/* hdbmk macros */
#define HDBMK_BUFSIZE 8192
#define HDBMK_NRECS   256

/* hfunc macros */
#define HFUNC_FNV32_BASIS ((uint32_t)0x0811c9dc5)
#define HFUNC_FNV32_PRIME ((uint32_t)0x01000193)
#define HFUNC_PHI32_PRIME ((uint32_t)0x09e3779b1)
#define hfunc_WXOR(h)     ((h) ^ ((h) >> 16))
#define hfunc_fnv1(k,l)   hfunc_ghfx((k), (l), HFUNC_FNV32_BASIS, HFUNC_FNV32_PRIME);
#define hfunc_nawk(k,l)   hfunc_kr31((k),(l))

/* ioq macros */
#define ioq_INIT(fd,buf,len,op) { (fd), (buf), (len), 0, (op) }
#define ioq_getc(ioq,c)         ioq_get((ioq),(uchar_t *)(c),1)
#define ioq_GETC(ioq,c)         (((ioq)->p > 0) ? *((uchar_t *)(c)) = (ioq)->buf[(ioq)->n], (ioq)->p -= 1, (ioq)->n += 1, 1 : ioq_get((ioq),(uchar_t *)(c),1))
#define ioq_PEEK(ioq)           ((ioq)->buf + (ioq)->n)
#define ioq_SEEK(ioq,nn)        { (ioq)->p -= (nn); (ioq)->n += (nn); }
#define ioq_putc(ioq,c)         ioq_put((ioq),&(c),1)
#define ioq_PUTC(ioq,c)         (((ioq)->p < (ioq)->n) ? ( (ioq)->buf[(ioq)->p++] = (c), 0 ) : ioq_put((ioq),&(c),1))
#define ioq_vputs(ioq, ...)     ioq_vputs_((ioq), __VA_ARGS__, NULL)
#define IOQ_BUFSIZE             8192

/* newenv macros */
#define newenv_unset(name) newenv_set((name), NULL)
#define newenv_RUN(av,ev)  newenv_exec(*(av)[0], (av), NULL, (ev))

/* nfmt macros */
#define NFMT_SIZE 40

/* outvec macros */
#define outvec_INIT(fd, vec, max, flushme) { (fd), (vec), 0, (max), (flushme) }
#define outvec_vputs(v, ...) outvec_vputs_((v), __VA_ARGS__, NULL)

/* packet macros */
#define packet_load(p, proto, type, len, data) p = { proto, type, len, data }

/* pkt macros */
#define PKT_HEADER        3
#define PKT_PAYLOAD       0xff
#define PKT_SIZE          (PKT_HEADER + PKT_PAYLOAD)
#define pkt_proto(k)      (uchar_t)((k)[0])
#define pkt_type(k)       (uchar_t)((k)[1])
#define pkt_len(k)        (size_t)(PKT_HEADER + (k)[2])
#define pkt_dlen(k)       (size_t)((k)[2])
#define pkt_data(k)       (uchar_t *)&(k)[3]
#define pkt_setproto(k,P) (k)[0]=(uchar_t)(P)
#define pkt_settype(k,T)  (k)[1]=(uchar_t)(T)
#define pkt_setdlen(k,N)  (k)[2]=(uint8_t)(N)
#define pkt_init(k,P,T,N) { pkt_setproto((k),(P)); pkt_settype((k),(T)); pkt_setdlen((k),(N)); }
#define pkt_INIT(P,T,N)   { (P), (T), (N) }

/* sig macros */
#define sig_uncatch(s) sig_catch((s), (SIG_DFL))
#define sig_default(s) sig_catch((s), (SIG_DFL))
#define sig_ignore(s)  sig_catch((s), (SIG_IGN))

/* sigset macros */
#define sigset_empty(sigset)    sigemptyset((sigset))
#define sigset_fill(sigset)     sigfillset((sigset))
#define sigset_add(sigset, sig) sigaddset((sigset), (sig))
#define sigset_del(sigset, sig) sigdelset((sigset), (sig))
#define sigset_block(sigset)    sigprocmask(SIG_BLOCK, (sigset), (sigset_t *)0)
#define sigset_unblock(sigset)  sigprocmask(SIG_UNBLOCK, (sigset), (sigset_t *)0)

/* sysstr macros */
#define EOK 0

/* tain macros */
#define TAIN_UTC_OFFSET   4611686018427387914ULL
#define TAIN_PACK_SIZE    12
#define TAIN_TAIPACK_SIZE 8
#define TAIN_HEXSTR_SIZE  (24 + 1)
#define tain_LOAD(t,s,n)  ((t)->sec = (uint64_t)(s), (t)->nsec = (uint32_t)(n))
#define tain_INIT(s,n)    { (uint64_t)(s), (uint32_t)(n) }

/* tx64 macros */
#define TX64_PAD   0x0
#define TX64_NOPAD 0x1

/* ufunc macros */
#define  UFUNC_U48MAX  (uint64_t)((1ULL<<48)-1)

/* upack macros */
#define upak16_UNPACK(b) \
(\
(((uint16_t)(((uchar_t *)(b))[1])) << 8) + \
  (uint16_t)(((uchar_t *)(b))[0])          \
)
#define upak24_UNPACK(b) \
(\
(((uint32_t)(((uchar_t *)(b))[2])) << 16) + \
(((uint32_t)(((uchar_t *)(b))[1])) << 8)  + \
  (uint32_t)(((uchar_t *)(b))[0])           \
)
#define upak32_UNPACK(b) \
(\
(((uint32_t)(((uchar_t *)(b))[3])) << 24) + \
(((uint32_t)(((uchar_t *)(b))[2])) << 16) + \
(((uint32_t)(((uchar_t *)(b))[1])) << 8)  + \
  (uint32_t)(((uchar_t *)(b))[0])           \
)
#define upak48_UNPACK(b) \
(\
(((uint64_t)(((uchar_t *)(b))[5])) << 40) + \
(((uint64_t)(((uchar_t *)(b))[4])) << 32) + \
(((uint64_t)(((uchar_t *)(b))[3])) << 24) + \
(((uint64_t)(((uchar_t *)(b))[2])) << 16) + \
(((uint64_t)(((uchar_t *)(b))[1])) << 8)  + \
  (uint64_t)(((uchar_t *)(b))[0])           \
)
#define upak64_UNPACK(b) \
(\
(((uint64_t)(((uchar_t *)(b))[7])) << 56) + \
(((uint64_t)(((uchar_t *)(b))[6])) << 48) + \
(((uint64_t)(((uchar_t *)(b))[5])) << 40) + \
(((uint64_t)(((uchar_t *)(b))[4])) << 32) + \
(((uint64_t)(((uchar_t *)(b))[3])) << 24) + \
(((uint64_t)(((uchar_t *)(b))[2])) << 16) + \
(((uint64_t)(((uchar_t *)(b))[1])) << 8)  + \
  (uint64_t)(((uchar_t *)(b))[0])           \
)

/* padlock macros */
enum padlock_wait {
	PADLOCK_NOW = 0,
	PADLOCK_WAIT
};

/* pidlock macros */
enum pidlock_wait {
	PIDLOCK_NOW = 0,
	PIDLOCK_WAIT
};

/* cdb types */
typedef struct cdb cdb_t;

struct cdb {
	int        fd;
	uchar_t  * map;
	uint32_t   map_size;
	uint32_t   hbase;
	uchar_t  * key;
	uint32_t   klen;
	uint32_t   h;
	uint32_t   tbase;
	uint32_t   tslots;
	uint32_t   s0;
	uint32_t   sN;
	uint32_t   rpos;
	uint32_t   dlen;
};

/* cdbmk types */
typedef struct cdbmk cdbmk_t;

struct cdbmk {
	int                  fd;
	off_t                fp;
	struct input_block * input_list[CDBMK_NLISTS];
	uint32_t             subtab_count[CDB_NTABS];
	struct ioq           ioq;
	uchar_t              buf[CDBMK_BUFSIZE];
};

/* dynbuf types */
typedef struct dynbuf dynbuf_t;

struct dynbuf {
	void   * buf;
	size_t   n;
	size_t   p;
};

/* dynstr types */
typedef struct dynstr dynstr_t;

struct dynstr {
	char   * s;
	size_t   n;
	size_t   k;
};

/* dynstuf types */
typedef struct dynstuf dynstuf_t;

struct dynstuf {
	void   ** stuf;
	size_t    slots;
	size_t    items;
};

/* hdb types */
typedef struct hdb_valoff hdb_valoff_t;

struct hdb_valoff{
	uint32_t value;
	uint32_t offset;
};

typedef struct hdb hdb_t;

struct hdb {
	int                 fd;
	uchar_t           * map;
	uint32_t            map_size;
	uint32_t            nrecs;
	struct hdb_valoff   tndx[HDB_NTABS];
	uint32_t            cbase;
	uint32_t            rbase;
	uint32_t            hbase;
	uchar_t           * key;
	uint32_t            klen;
	uint32_t            h;
	uint32_t            tbase;
	uint32_t            tslots;
	uint32_t            s0;
	uint32_t            sN;
	uint32_t            rpos;
	uint32_t            dlen;
};

/* hdbmk types */
typedef struct hdbmk hdbmk_t;

struct hdbmk {
	int                  fd;
	off_t                fp;
	uint32_t             rbase;
	struct hdbmk_block * block_list[HDB_NTABS];
	uint32_t             subtab_count[HDB_NTABS];
	struct ioq           ioq;
	uchar_t              buf[HDBMK_BUFSIZE];
};

typedef struct hdbmk_block hdbmk_block_t;

struct hdbmk_block {
	struct hdb_valoff    record[HDBMK_NRECS];
	int                  n;
	struct hdbmk_block * next;
};

/* ioq types */
typedef struct ioq ioq_t;

struct ioq {
	int       fd;
	uchar_t * buf;
	size_t    n;
	size_t    p;
	ssize_t   (*op)();
};

/* outvec types */
typedef struct outvec outvec_t;

struct outvec {
	int            fd;
	struct iovec * vec;
	size_t         n;
	size_t         max;
	int            flushme;
};

/* packet types */
typedef struct packet packet_t;

struct packet {
	char      proto_id;
	char      type_id;
	uint8_t   len;
	char    * data;
};

/* pkt types */
typedef uchar_t pkt_t[PKT_SIZE];

/* sig types */
typedef void (*sig_handler_t)(int);

/* tain types */
typedef struct tain tain_t;

struct tain {
	uint64_t sec;
	uint32_t nsec;
};

/* uchar types */
typedef unsigned char uchar_t;

/* buf routines */
extern int buf_cmp(const void *, const void *, size_t);
extern void * buf_fill(void *, size_t, int);
extern void * buf_copy(void *, const void *, size_t);
extern void * buf_rcopy(void *, const void *, size_t);
extern size_t buf_ndx(const void *, size_t, int);
extern size_t buf_rndx(const void *, size_t, int);

/* cdb routines */
extern int cdb_init(cdb_t *, int);
extern int cdb_clear(cdb_t *);
extern int cdb_open(cdb_t *, const char *);
extern int cdb_close(cdb_t *);
extern int cdb_find(cdb_t *, const uchar_t *, uint32_t);
extern int cdb_findnext(cdb_t *);
extern int cdb_get(cdb_t *, uchar_t *, size_t);
extern int cdb_dynget(cdb_t *, dynbuf_t *);
extern uint32_t cdb_distance(cdb_t *);
extern int cdb_seqinit(cdb_t *);
extern int cdb_seqnext(cdb_t *);
extern int cdb_read(cdb_t *, uchar_t *, size_t, uint32_t);
extern int cdb_dynread(cdb_t *, dynbuf_t *, size_t, uint32_t);
extern cdb_t * cdb_cc(cdb_t *, const cdb_t *);
extern uint32_t cdb_hashpart(uint32_t, const uchar_t *, size_t);

/* cdbmk routines */
extern int cdbmk_init(cdbmk_t *, int);
extern int cdbmk_add(cdbmk_t *, const uchar_t *, uint32_t, const uchar_t *, uint32_t);
extern int cdbmk_addioq(cdbmk_t *, ioq_t *, size_t, size_t, uchar_t *, size_t);
extern int cdbmk_addrec(cdbmk_t *, cdb_t *);
extern int cdbmk_finish(cdbmk_t *);
extern void cdbmk_clear(cdbmk_t *);
extern int cdbmk__update(struct cdbmk *, uint32_t, uint32_t, uint32_t);

/* cstr routines */
extern size_t cstr_cat(char *, const char *);
extern char * cstr_chop(char *);
extern int cstr_cmp(const char *, const char *);
extern int cstr_cmpi(const char *, const char *);
extern int cstr_contains(const char *, const char *);
extern size_t cstr_copy(char *, const char *);
extern char * cstr_dup(const char *);
extern size_t cstr_lcat(char *, const char *, size_t);
extern size_t cstr_lcpy(char *, const char *, size_t);
extern size_t cstr_len(const char *);
extern char * cstr_ltrim(char *);
extern int cstr_match(const char *, const char *);
extern int cstr_matchi(const char *, const char *);
extern int cstr_ncmp(const char *, const char *, size_t);
extern size_t cstr_pos(const char *, int);
extern size_t cstr_rpos(const char *, int);
extern char * cstr_rtrim(char *);
extern char * cstr_trim(char *);
extern size_t cstr_vcat_(char *, ...);
extern size_t cstr_vcopy_(char *, ...);
extern size_t cstr_vlen_(const char *, ...);

/* devout routines */
extern ssize_t devout_(int, const char *, ...);

/* domsock routines */
extern int domsock_create(const char *, mode_t);
extern int domsock_accept(int);
extern int domsock_close(int);
extern int domsock_connect(const char *);

/* dynbuf routines */
extern dynbuf_t * dynbuf_new(void);
extern void dynbuf_free(dynbuf_t *);
extern void dynbuf_freebuf(dynbuf_t *);
extern void dynbuf_clear(dynbuf_t *);
extern void * dynbuf_buf(dynbuf_t *);
extern size_t dynbuf_len(dynbuf_t *);
extern int dynbuf_need(dynbuf_t *, size_t);
extern int dynbuf_grow(dynbuf_t *, size_t);
extern int dynbuf_put(dynbuf_t *, const dynbuf_t *);
extern int dynbuf_putbuf(dynbuf_t *, const void *, size_t);
extern int dynbuf_putb(dynbuf_t *, unsigned char);
extern int dynbuf_puts(dynbuf_t *, const char *);
extern int dynbuf_putnul(dynbuf_t *);
extern int dynbuf_copy(dynbuf_t *, const dynbuf_t *);
extern int dynbuf_copybuf(dynbuf_t *, const void *, size_t);
extern int dynbuf_copys(dynbuf_t *, const char *);
extern ssize_t dynbuf_pack(dynbuf_t *, const char *, ...);

/* dynstr routines */
extern void dynstr_set(dynstr_t *, char *);
extern dynstr_t * dynstr_new(void);
extern void dynstr_free(dynstr_t *);
extern void dynstr_freestr(dynstr_t *);
extern void dynstr_clear(dynstr_t *);
extern char * dynstr_str(dynstr_t *);
extern size_t dynstr_len(dynstr_t *);
extern int dynstr_need(dynstr_t *, size_t);
extern int dynstr_grow(dynstr_t *, size_t);
extern int dynstr_put(dynstr_t *, dynstr_t *);
extern int dynstr_puts(dynstr_t *, const char *);
extern int dynstr_putn(dynstr_t *, const char *, size_t);
extern int dynstr_putc(dynstr_t *, char);
extern int dynstr_vputs_(dynstr_t *, const char *, ...);
extern int dynstr_copy(dynstr_t *, dynstr_t *);
extern int dynstr_copys(dynstr_t *, const char *);
extern void dynstr_chop(dynstr_t *);

/* dynstuf routines */
extern struct dynstuf * dynstuf_new(void);
extern struct dynstuf * dynstuf_init(struct dynstuf *);
extern int dynstuf_grow(struct dynstuf *, size_t);
extern void dynstuf_free(struct dynstuf *, void (*)(void *));
extern size_t dynstuf_items(struct dynstuf *);
extern size_t dynstuf_isempty(struct dynstuf *);
extern size_t dynstuf_slots(struct dynstuf *);
extern void ** dynstuf_stuf(struct dynstuf *);
extern int dynstuf_push(struct dynstuf *, void *);
extern void * dynstuf_peek(struct dynstuf *);
extern void * dynstuf_pop(struct dynstuf *);
extern void * dynstuf_get(struct dynstuf *, size_t);
extern void * dynstuf_set(struct dynstuf *, size_t, void *);
extern void * dynstuf_replace(struct dynstuf *, size_t, void *);
extern void dynstuf_sort(struct dynstuf *, int(*)(const void *, const void *));
extern void dynstuf_reverse(struct dynstuf *);
extern void dynstuf_visit(struct dynstuf *, void (*)(void *, void *), void *);

/* execvx routines */
extern int execvx(const char *, char *const *, char *const *, const char *);

/* fd routines */
extern int fd_cloexec(int);
extern int fd_nonblock(int);
extern int fd_blocking(int);
extern int fd_dupe(int, int);
extern int fd_move(int, int);

/* hdb routines */
extern int hdb_init(hdb_t *, int);
extern int hdb_clear(hdb_t *);
extern int hdb_open(hdb_t *, const char *);
extern int hdb_close(hdb_t *);
extern int hdb_find(hdb_t *, const uchar_t *, uint32_t);
extern int hdb_findnext(hdb_t *);
extern int hdb_get(hdb_t *, uchar_t *, size_t);
extern int hdb_dynget(hdb_t *, dynbuf_t *);
extern uint32_t hdb_distance(hdb_t *);
extern int hdb_seqinit(hdb_t *);
extern int hdb_seqnext(hdb_t *);
extern int hdb_read(hdb_t *, uchar_t *, size_t, uint32_t);
extern int hdb_dynread(hdb_t *, dynbuf_t *, size_t, uint32_t);
extern hdb_t * hdb_cc(hdb_t *, const hdb_t *);
extern uint32_t hdb_hashpart(uint32_t, const uchar_t *, size_t);

/* hdbmk routines */
extern int hdbmk_start(hdbmk_t *, int, const uchar_t *, uint32_t);
extern int hdbmk_add(hdbmk_t *, const uchar_t *, uint32_t, const uchar_t *, uint32_t);
extern int hdbmk_addioq(hdbmk_t *, ioq_t *, size_t, size_t, uchar_t *, size_t);
extern int hdbmk_addrec(hdbmk_t *, hdb_t *);
extern int hdbmk_finish(hdbmk_t *);
extern void hdbmk_clear(hdbmk_t *);
extern int hdbmk__update(struct hdbmk *, uint32_t, uint32_t, uint32_t);

/* hfunc routines */
extern uint32_t hfunc_postmix32(uint32_t);
extern uint32_t hfunc_ghfa(const uchar_t *, size_t, uint32_t, uint32_t);
extern uint32_t hfunc_ghfx(const uchar_t *, size_t, uint32_t, uint32_t);
extern uint32_t hfunc_ghfm(const uchar_t *, size_t, uint32_t, uint32_t);
extern uint32_t hfunc_djba(const uchar_t *, size_t);
extern uint32_t hfunc_djbx(const uchar_t *, size_t);
extern uint32_t hfunc_kp37(const uchar_t *, size_t);
extern uint32_t hfunc_kr31(const uchar_t *, size_t);
extern uint32_t hfunc_p50a(const uchar_t *, size_t);
extern uint32_t hfunc_sdbm(const uchar_t *, size_t);
extern uint32_t hfunc_djbm(const uchar_t *, size_t);
extern uint32_t hfunc_elf1(const uchar_t *, size_t);
extern uint32_t hfunc_fnva(const uchar_t *, size_t);
extern uint32_t hfunc_fnvm(const uchar_t *, size_t);
extern uint32_t hfunc_jsw1(const uchar_t *, size_t);
extern uint32_t hfunc_kx17(const uchar_t *, size_t);
extern uint32_t hfunc_murm(const uchar_t *, size_t);
extern uint32_t hfunc_oat1(const uchar_t *, size_t);
extern uint32_t hfunc_pjw1(const uchar_t *, size_t);
extern uint32_t hfunc_rot1(const uchar_t *, size_t);
extern uint32_t hfunc_rotm(const uchar_t *, size_t);
extern uint32_t hfunc_rsuh(const uchar_t *, size_t);
extern uint32_t hfunc_sax1(const uchar_t *, size_t);
extern uint32_t hfunc_sfh1(const uchar_t *, size_t);

/* ioq routines */
extern void ioq_init(ioq_t *, int, uchar_t *, size_t, ssize_t (*)());
extern ssize_t ioq_get(ioq_t *, uchar_t *, size_t);
extern int ioq_getln(ioq_t *, dynstr_t *);
extern ssize_t ioq_feed(ioq_t *);
extern void * ioq_peek(ioq_t *);
extern void ioq_seek(ioq_t *, size_t);
extern int ioq_flush(ioq_t *);
extern int ioq_put(ioq_t *, const uchar_t *, size_t);
extern int ioq_putfd(ioq_t *, int, size_t);
extern int ioq_putfile(ioq_t *, const char *);
extern int ioq_puts(ioq_t *, const char *);
extern int ioq_vputs_(ioq_t *, const char *, ...);
extern int ioq_putflush(ioq_t *, const uchar_t *, size_t);
extern int ioq_putsflush(ioq_t *, const char *);
extern int ioq_putfill(ioq_t *, const uchar_t *, size_t);
extern int ioq_putsfill(ioq_t *, const char *);

/* newenv routines */
extern int newenv_set(const char *, const char *);
extern int newenv_run(char *const *, char *const *);
extern int newenv_exec(const char *, char *const *, const char *, char *const *);

/* nfmt routines */
extern char * nfmt_uint32(char *, uint32_t);
extern char * nfmt_uint64(char *, uint64_t);
extern char * nfmt_uint32_pad(char *, uint32_t, size_t);
extern char * nfmt_uint64_pad(char *, uint64_t, size_t);
extern char * nfmt_uint32o(char *, uint32_t);
extern char * nfmt_uint32x(char *, uint32_t);
extern char * nfmt_uint32_pad0(char *, uint32_t, size_t);
extern char * nfmt_uint64_pad0(char *, uint64_t, size_t);
extern char * nfmt_uint32o_pad0(char *, uint32_t, size_t);
extern char * nfmt_uint32x_pad(char *, uint32_t, size_t);
extern char * nfmt_uint32x_pad0(char *, uint32_t, size_t);
extern size_t nfmt_uint32_(char *, uint32_t);
extern size_t nfmt_uint64_(char *, uint64_t);
extern size_t nfmt_uint32o_(char *, uint32_t);
extern size_t nfmt_uint32x_(char *, uint32_t);
extern size_t nfmt_uint32_pad_(char *, uint32_t, size_t);
extern size_t nfmt_uint64_pad_(char *, uint64_t, size_t);
extern size_t nfmt_uint32x_pad_(char *, uint32_t, size_t);
extern size_t nfmt_uint32_pad0_(char *, uint32_t, size_t);
extern size_t nfmt_uint64_pad0_(char *, uint64_t, size_t);
extern size_t nfmt_uint32o_pad0_(char *, uint32_t, size_t);
extern size_t nfmt_uint32x_pad0_(char *, uint32_t, size_t);

/* nuscan routines */
extern const char * nuscan_uint32(uint32_t *, const char *);
extern const char * nuscan_uint32o(uint32_t *, const char *);

/* outvec routines */
extern int outvec_put(struct outvec *, const void *, size_t);
extern int outvec_puts(struct outvec *, const char *);
extern int outvec_vputs_(struct outvec *, const char *, ...);
extern int outvec_flush(struct outvec *);

/* packet routines */
extern ssize_t packet_read(int, void *, size_t);
extern ssize_t packet_write(int, const void *, size_t);

/* padlock routines */
extern int padlock_fcntl(int, int, struct flock *);
extern int padlock_exlock(int, enum padlock_wait);
extern int padlock_shlock(int, enum padlock_wait);
extern int padlock_unlock(int, enum padlock_wait);
extern pid_t padlock_extest(int);
extern pid_t padlock_shtest(int);
extern int padlock_exbyte(int, off_t, enum padlock_wait);
extern int padlock_shbyte(int, off_t, enum padlock_wait);
extern int padlock_unbyte(int, off_t, enum padlock_wait);

/* pidlock routines */
extern pid_t pidlock_check(const char *);
extern int pidlock_set(const char *, pid_t, enum pidlock_wait);

/* pkt routines */
extern int pkt_load(pkt_t, uchar_t, uchar_t, uchar_t *, size_t);
extern ssize_t pkt_read(int, pkt_t, size_t);
extern ssize_t pkt_write(int, const pkt_t, size_t);

/* pollio routines */
extern int pollio(struct pollfd *, nfds_t, int, int *);

/* rlimit routines */
extern int rlimit_lookup(const char *);
extern const char * rlimit_name(int);
extern const char * rlimit_mesg(int);

/* sig routines */
extern void sig_block(int);
extern void sig_unblock(int);
extern sig_handler_t sig_catch(int, sig_handler_t);
extern sig_handler_t sig_catchr(int, sig_handler_t);

/* sysstr routines */
extern const char * sysstr_errno(int);
extern const char * sysstr_errno_mesg(int);
extern const char * sysstr_signal(int);
extern const char * sysstr_signal_mesg(int);

/* tain routines */
extern tain_t * tain_load(tain_t *, uint64_t, uint32_t);
extern tain_t * tain_now(tain_t *);
extern tain_t * tain_assign(tain_t *, const tain_t *);
extern tain_t * tain_load_utc(tain_t *, time_t);
extern time_t tain_to_utc(tain_t *);
extern tain_t * tain_load_msecs(tain_t *, uint64_t);
extern uint64_t tain_to_msecs(tain_t *);
extern double tain_to_float(tain_t *);
extern tain_t * tain_plus(tain_t *, const tain_t *, const tain_t *);
extern tain_t * tain_minus(tain_t *, const tain_t *, const tain_t *);
extern int tain_less(const tain_t *, const tain_t *);
extern int tain_iszero(const tain_t *);
extern uint64_t tain_uptime(const tain_t *, const tain_t *);
extern uchar_t * tain_pack(uchar_t *, const tain_t *);
extern tain_t * tain_unpack(tain_t *, const uchar_t *);
extern char * tain_packhex(char *, const tain_t *);
extern tain_t * tain_unpackhex(tain_t *, const char *);
extern uchar_t * tain_tai_pack(uchar_t *, const tain_t *);
extern tain_t * tain_tai_unpack(tain_t *, const uchar_t *);
extern int tain_pause(const tain_t *, tain_t *);

/* tx64 routines */
extern size_t tx64_encode (char *, const char *, size_t, const char *, int);

/* ufunc routines */
extern int ufunc_u32add(uint32_t *, uint32_t);
extern int ufunc_u48add(uint64_t *, uint64_t);

/* upak routines */
extern uchar_t * upak16_pack(uchar_t *, uint16_t);
extern uint16_t upak16_unpack(const uchar_t *);
extern uchar_t * upak24_pack(uchar_t *, uint32_t);
extern uint32_t upak24_unpack(const uchar_t *);
extern uchar_t * upak32_pack(uchar_t *, uint32_t);
extern uint32_t upak32_unpack(const uchar_t *);
extern uchar_t * upak48_pack(uchar_t *, uint64_t);
extern uint64_t upak48_unpack(const uchar_t *);
extern uchar_t * upak64_pack(uchar_t *, uint64_t);
extern uint64_t upak64_unpack(const uchar_t *);
extern ssize_t upak_pack(uchar_t *, const char *, ...);
extern ssize_t upak_vpack(uchar_t *, const char *, va_list);
extern ssize_t upak_unpack(uchar_t *, const char *, ...);

/* ioq_std variables */
extern ioq_t * ioq0;
extern ioq_t * ioq1;
extern ioq_t * ioq2;

/* outvec variables */
extern struct outvec OUTVEC_STDOUT;
extern struct outvec OUTVEC_STDERR;

/* tx64 variables */
extern const char base64_vec[64];
