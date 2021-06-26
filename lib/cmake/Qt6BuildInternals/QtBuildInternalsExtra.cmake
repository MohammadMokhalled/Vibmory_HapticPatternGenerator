# Propagate common variables via BuildInternals package.
set(QT_BUILD_SHARED_LIBS OFF)
option(BUILD_SHARED_LIBS "Build Qt statically or dynamically" OFF)
set(QT_CMAKE_EXPORT_NAMESPACE Qt6)
set(INSTALL_CMAKE_NAMESPACE Qt6)
set(QT_BUILD_INTERNALS_PATH "${CMAKE_CURRENT_LIST_DIR}")

# The relocatable install prefix is meant to be used to find things like host binaries (syncqt),
# when the CMAKE_INSTALL_PREFIX is overridden to point to a different path (like when building a
# a Qt repo using Conan, which will set a random install prefix instead of installing into the
# original Qt install prefix).
get_filename_component(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX
                       ${CMAKE_CURRENT_LIST_DIR}/../../../
                       ABSOLUTE)

# If no explicit CMAKE_INSTALL_PREFIX is provided, force set the original Qt installation prefix,
# so that further modules / repositories are  installed into same original location.
# This means by default when configuring qtsvg / qtdeclarative, they will be installed the regular
# Qt installation prefix.
# If an explicit installation prefix is specified,  honor it.
# This is an attempt to support Conan, aka handle installation of modules into a
# different installation prefix than the original one. Also allow to opt out via a special variable.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND
   NOT QT_BUILD_INTERNALS_NO_FORCE_SET_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/QtBase" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
endif()

# Propagate developer builds to other modules via BuildInternals package.
if(OFF)
    set(FEATURE_developer_build ON CACHE BOOL "Developer build." FORCE)
endif()

# Propagate non-prefix builds.
set(QT_WILL_INSTALL ON CACHE BOOL
    "Boolean indicating if doing a Qt prefix build (vs non-prefix build)." FORCE)

set(QT_SOURCE_TREE "C:/Qt/6.0.3/Src/qtbase" CACHE PATH
"A path to the source tree of the previously configured QtBase project." FORCE)

# Propagate decision of building tests and examples to other repositories.
set(QT_BUILD_TESTS OFF CACHE BOOL "Build the testing tree.")
set(QT_BUILD_EXAMPLES OFF CACHE BOOL "Build Qt examples")
set(QT_BUILD_TESTS_BY_DEFAULT ON CACHE BOOL
    "Should tests be built as part of the default 'all' target.")
set(QT_BUILD_EXAMPLES_BY_DEFAULT ON CACHE BOOL
    "Should examples be built as part of the default 'all' target.")

# Propagate usage of ccache.
set(QT_USE_CCACHE OFF CACHE BOOL "Enable the use of ccache")

# Extra set of exported variables
set(TEST_architecture_arch "x86_64" CACHE INTERNAL "")
set(TEST_subarch_result "cx16;mmx;sse;sse2;sse3" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_cx16 "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_mmx "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_sse "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_sse2 "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_sse3 "1" CACHE INTERNAL "")
set(TEST_buildAbi "x86_64-little_endian-lp64" CACHE INTERNAL "")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
set(BUILD_WITH_PCH "ON" CACHE STRING "")
set(QT_QPA_DEFAULT_PLATFORM "windows" CACHE STRING "")
set(QT_MIN_SUPPORTED_CMAKE_VERSION "3.16" CACHE STRING "Minimum supported CMake version required to build Qt")
set(QT_COMPUTED_MIN_CMAKE_VERSION "3.16" CACHE STRING "Computed minimum CMake version required to build Qt")
set(QT_MIN_NEW_POLICY_CMAKE_VERSION "3.14" CACHE STRING "Oldest CMake version for which NEW policies should be enabled")
set(QT_MAX_NEW_POLICY_CMAKE_VERSION "3.19" CACHE STRING "Latest CMake version for which NEW policies should be enabled")
set(CMAKE_INSTALL_RPATH "" CACHE STRING "")
set(QT_EXTRA_DEFINES "QT_BUILD_TESTS_BY_DEFAULT=OFF" CACHE STRING "")

if(NOT QT_SKIP_BUILD_INTERNALS_PKG_CONFIG_FEATURE)
    set(FEATURE_pkg_config "OFF" CACHE STRING "Using pkg-config" FORCE)
endif()

set(INSTALL_BINDIR "bin" CACHE STRING "Executables [PREFIX/bin]" FORCE)
set(INSTALL_INCLUDEDIR "include" CACHE STRING "Header files [PREFIX/include]" FORCE)
set(INSTALL_LIBDIR "lib" CACHE STRING "Libraries [PREFIX/lib]" FORCE)
set(INSTALL_MKSPECSDIR "mkspecs" CACHE STRING "Mkspecs files [PREFIX/mkspecs]" FORCE)
set(INSTALL_ARCHDATADIR "." CACHE STRING "Arch-dependent data [PREFIX]" FORCE)
set(INSTALL_PLUGINSDIR "plugins" CACHE STRING "Plugins [ARCHDATADIR/plugins]" FORCE)
set(INSTALL_LIBEXECDIR "bin" CACHE STRING "Helper programs [ARCHDATADIR/bin on Windows, ARCHDATADIR/libexec otherwise]" FORCE)
set(INSTALL_QMLDIR "qml" CACHE STRING "QML2 imports [ARCHDATADIR/qml]" FORCE)
set(INSTALL_DATADIR "." CACHE STRING "Arch-independent data [PREFIX]" FORCE)
set(INSTALL_DOCDIR "doc" CACHE STRING "Documentation [DATADIR/doc]" FORCE)
set(INSTALL_TRANSLATIONSDIR "translations" CACHE STRING "Translations [DATADIR/translations]" FORCE)
set(INSTALL_SYSCONFDIR "etc/xdg" CACHE STRING "Settings used by Qt programs [PREFIX/etc/xdg]/[/Library/Preferences/Qt]" FORCE)
set(INSTALL_EXAMPLESDIR "examples" CACHE STRING "Examples [PREFIX/examples]" FORCE)
set(INSTALL_TESTSDIR "tests" CACHE STRING "Tests [PREFIX/tests]" FORCE)
set(INSTALL_DESCRIPTIONSDIR "modules" CACHE STRING "Module description files directory" FORCE)

