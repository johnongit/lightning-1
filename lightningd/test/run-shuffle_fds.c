#include "config.h"
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#undef dup2
#undef close
#undef fcntl

#define dup2 test_dup2
#define close test_close
#define fcntl test_fcntl

/* Indexed by fd num, -1 == not open. */
#define MAX_TEST_FDS 100
static int test_fd_arr[MAX_TEST_FDS];

static int test_dup2(int oldfd, int newfd)
{
	/* Must not clobber an existing fd */
	assert(test_fd_arr[newfd] == -1);
	test_fd_arr[newfd] = test_fd_arr[oldfd];
	return 0;
}

static int test_close(int fd)
{
	assert(test_fd_arr[fd] != -1);
	test_fd_arr[fd] = -1;
	return 0;
}

static int test_fcntl(int fd, int cmd, ... /* arg */ )
{
	assert(test_fd_arr[fd] != -1);
	return 0;
}

#include "../subd.c"
#include <common/json_stream.h>
#include <common/setup.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for db_begin_transaction_ */
void db_begin_transaction_(struct db *db UNNEEDED, const char *location UNNEEDED)
{ fprintf(stderr, "db_begin_transaction_ called!\n"); abort(); }
/* Generated stub for db_commit_transaction */
void db_commit_transaction(struct db *db UNNEEDED)
{ fprintf(stderr, "db_commit_transaction called!\n"); abort(); }
/* Generated stub for db_in_transaction */
bool db_in_transaction(struct db *db UNNEEDED)
{ fprintf(stderr, "db_in_transaction called!\n"); abort(); }
/* Generated stub for fatal */
void   fatal(const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "fatal called!\n"); abort(); }
/* Generated stub for fromwire_bigsize */
bigsize_t fromwire_bigsize(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_bigsize called!\n"); abort(); }
/* Generated stub for fromwire_channel_id */
bool fromwire_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			 struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_channel_id called!\n"); abort(); }
/* Generated stub for fromwire_node_id */
void fromwire_node_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct node_id *id UNNEEDED)
{ fprintf(stderr, "fromwire_node_id called!\n"); abort(); }
/* Generated stub for fromwire_status_fail */
bool fromwire_status_fail(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, enum status_failreason *failreason UNNEEDED, wirestring **desc UNNEEDED)
{ fprintf(stderr, "fromwire_status_fail called!\n"); abort(); }
/* Generated stub for fromwire_status_peer_billboard */
bool fromwire_status_peer_billboard(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, bool *perm UNNEEDED, wirestring **happenings UNNEEDED)
{ fprintf(stderr, "fromwire_status_peer_billboard called!\n"); abort(); }
/* Generated stub for fromwire_status_peer_error */
bool fromwire_status_peer_error(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, struct channel_id *channel UNNEEDED, wirestring **desc UNNEEDED, bool *warning UNNEEDED, u8 **error_for_them UNNEEDED)
{ fprintf(stderr, "fromwire_status_peer_error called!\n"); abort(); }
/* Generated stub for fromwire_status_version */
bool fromwire_status_version(const tal_t *ctx UNNEEDED, const void *p UNNEEDED, wirestring **version UNNEEDED)
{ fprintf(stderr, "fromwire_status_version called!\n"); abort(); }
/* Generated stub for log_ */
void log_(struct log *log UNNEEDED, enum log_level level UNNEEDED,
	  const struct node_id *node_id UNNEEDED,
	  bool call_notifier UNNEEDED,
	  const char *fmt UNNEEDED, ...)

{ fprintf(stderr, "log_ called!\n"); abort(); }
/* Generated stub for log_prefix */
const char *log_prefix(const struct log *log UNNEEDED)
{ fprintf(stderr, "log_prefix called!\n"); abort(); }
/* Generated stub for log_print_level */
enum log_level log_print_level(struct log *log UNNEEDED, const struct node_id *node_id UNNEEDED)
{ fprintf(stderr, "log_print_level called!\n"); abort(); }
/* Generated stub for log_status_msg */
bool log_status_msg(struct log *log UNNEEDED,
 		    const struct node_id *node_id UNNEEDED,
		    const u8 *msg UNNEEDED)
{ fprintf(stderr, "log_status_msg called!\n"); abort(); }
/* Generated stub for new_log */
struct log *new_log(const tal_t *ctx UNNEEDED, struct log_book *record UNNEEDED,
		    const struct node_id *default_node_id UNNEEDED,
		    const char *fmt UNNEEDED, ...)
{ fprintf(stderr, "new_log called!\n"); abort(); }
/* Generated stub for new_peer_fd_arr */
struct peer_fd *new_peer_fd_arr(const tal_t *ctx UNNEEDED, const int *fd UNNEEDED)
{ fprintf(stderr, "new_peer_fd_arr called!\n"); abort(); }
/* Generated stub for subdaemon_path */
const char *subdaemon_path(const tal_t *ctx UNNEEDED, const struct lightningd *ld UNNEEDED, const char *name UNNEEDED)
{ fprintf(stderr, "subdaemon_path called!\n"); abort(); }
/* Generated stub for towire_bigsize */
void towire_bigsize(u8 **pptr UNNEEDED, const bigsize_t val UNNEEDED)
{ fprintf(stderr, "towire_bigsize called!\n"); abort(); }
/* Generated stub for towire_channel_id */
void towire_channel_id(u8 **pptr UNNEEDED, const struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "towire_channel_id called!\n"); abort(); }
/* Generated stub for towire_node_id */
void towire_node_id(u8 **pptr UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "towire_node_id called!\n"); abort(); }
/* Generated stub for version */
const char *version(void)
{ fprintf(stderr, "version called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

static void run_test_(int fd0, ...)
{
	va_list ap;
	int fd, i;
	int *test_fds = tal_arr(tmpctx, int, 1);
	int **test_fd_ptrs;

	/* They start all closed */
	memset(test_fd_arr, 0xFF, sizeof(test_fd_arr));

	test_fds[0] = fd0;
	test_fd_arr[fd0] = fd0;

	va_start(ap, fd0);
	while ((fd = va_arg(ap, int)) != -1) {
		tal_arr_expand(&test_fds, fd);
		test_fd_arr[fd] = fd;
	}
	va_end(ap);

	test_fd_ptrs = tal_arr(tmpctx, int *, tal_count(test_fds));
	for (i = 0; i < tal_count(test_fds); i++)
		test_fd_ptrs[i] = &test_fds[i];

	assert(shuffle_fds(test_fd_ptrs, tal_count(test_fd_ptrs)));

	/* Make sure fds ended up where expected */
	i = 0;
	assert(test_fd_arr[i++] == fd0);
	va_start(ap, fd0);
	while ((fd = va_arg(ap, int)) != -1)
		assert(test_fd_arr[i++] == fd);
	va_end(ap);

	/* And rest were closed */
	for (; i < MAX_TEST_FDS; i++)
		assert(test_fd_arr[i] == -1);
}

#define run_test(...) \
	run_test_(__VA_ARGS__, -1)

int main(int argc, char *argv[])
{
	common_setup(argv[0]);

	run_test(0);
	run_test(1);
	run_test(0, 1);
	run_test(0, 1, 3);
	run_test(3, 2, 1, 0);
	run_test(5, 2, 1);

	common_shutdown();
}
