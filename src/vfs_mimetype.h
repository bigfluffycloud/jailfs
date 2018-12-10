#if	!defined(__mimetypes_h)
#define	__mimetypes_h
#include <magic.h>
extern magic_t mimetype_init(void);
extern void mimetype_fini(magic_t ptr);
extern const char *mimetype_by_path(magic_t ptr, const char *path);
extern const char *mimetype_by_fd(magic_t ptr, int fd);
#endif	// !defined(__mimetypes_h)
