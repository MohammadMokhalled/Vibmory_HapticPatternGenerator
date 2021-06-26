/* Installation date */
static const char qt_configure_installation     [12+11]  = "qt_instdate=2012-12-20";
/* Installation Info */
static const char qt_configure_prefix_path_str  [12+256] = "qt_prfxpath=C:/Program Files (x86)/QtBase";
#ifdef QT_BUILD_QMAKE
static const char qt_configure_ext_prefix_path_str   [12+256] = "qt_epfxpath=C:/Program Files (x86)/QtBase";
static const char qt_configure_host_prefix_path_str  [12+256] = "qt_hpfxpath=C:/Program Files (x86)/QtBase";
#endif
static const short qt_configure_str_offsets[] = {
    0,
    4,
    12,
    16,
    20,
    24,
    32,
    36,
    38,
    40,
    53,
    62,

#ifdef QT_BUILD_QMAKE
    68,
    69,
    75,
    79,
    83,
    85,
    96,

#endif
};
static const char qt_configure_strs[] =
    "doc\0"
    "include\0"
    "lib\0"
    "bin\0"
    "bin\0"
    "plugins\0"
    "qml\0"
    ".\0"
    ".\0"
    "translations\0"
    "examples\0"
    "tests\0"

#ifdef QT_BUILD_QMAKE
    "\0"
    "false\0"
    "bin\0"
    "lib\0"
    ".\0"
    "win32-msvc\0"
    "win32-msvc\0"

#endif
;
#define QT_CONFIGURE_SETTINGS_PATH "etc/xdg"
#define QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH "../"
#define QT_CONFIGURE_HOSTBINDIR_TO_EXTPREFIX_PATH "../"
#define QT_CONFIGURE_HOSTBINDIR_TO_HOSTPREFIX_PATH "../"
#ifdef QT_BUILD_QMAKE
# define QT_CONFIGURE_CROSSBUILD 0
# define QT_CONFIGURE_SYSROOTIFY_PREFIX false
#endif
#define QT_CONFIGURE_PREFIX_PATH qt_configure_prefix_path_str + 12
#ifdef QT_BUILD_QMAKE
# define QT_CONFIGURE_EXT_PREFIX_PATH qt_configure_ext_prefix_path_str + 12
# define QT_CONFIGURE_HOST_PREFIX_PATH qt_configure_host_prefix_path_str + 12
#endif
