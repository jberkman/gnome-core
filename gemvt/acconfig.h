#undef	ENABLE_NLS
#undef	HAVE_CATGETS
#undef	HAVE_GETTEXT
#undef	HAVE_LC_MESSAGES
#undef	HAVE_STPCPY
#undef	HAVE_WAIT4
#undef	HAVE_LIBSM

/* build with consideration of "linux" entry in termcap?
 */
#undef	HAVE_TERMCAP_LINUX

/+ perform GLE initialisation?
*/
#undef	HAVE_LIBGLE

/* build for the GNOME desktop project ?
 */
#undef	HAVE_GNOME

#undef	PACKAGE
#undef	GEMVT_MAJOR
#undef	GEMVT_REVISION
#undef	GEMVT_PATCHLEVEL
#undef	GEMVT_VERSION
#undef	VERSION

@BOTTOM@

/* general defines
 */
#define	PRGNAME		"GemVT"
#define	PRGNAME_LONG 	PRGNAME " - GNU Emulator of a Virtual Terminal"
