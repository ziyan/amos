CMAKE_MINIMUM_REQUIRED (VERSION 2.4 FATAL_ERROR)
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY (SET CMP0003 NEW)
    IF (POLICY CMP0011)
        CMAKE_POLICY (SET CMP0011 NEW)
    ENDIF (POLICY CMP0011)
ENDIF (COMMAND CMAKE_POLICY)

INCLUDE (FindPkgConfig)
IF (NOT PKG_CONFIG_FOUND)
    SET (SQLITE3_CFLAGS "")
    SET (SQLITE3_INCLUDE_DIRS )
    LIST (APPEND SQLITE3_INCLUDE_DIRS "/usr/include")
    SET (SQLITE3_LINK_LIBS m;z)
    LIST (APPEND SQLITE3_LINK_LIBS "sqlite3")
    SET (SQLITE3_LIBRARY_DIRS )
    LIST (APPEND SQLITE3_LIBRARY_DIRS "/usr/lib")
    SET (SQLITE3_LINK_FLAGS "")
ELSE (NOT PKG_CONFIG_FOUND)
    pkg_check_modules (SQLITE3_PKG REQUIRED sqlite3)
    IF (SQLITE3_PKG_CFLAGS_OTHER)
        LIST_TO_STRING (SQLITE3_CFLAGS "${SQLITE3_PKG_CFLAGS_OTHER}")
    ELSE (SQLITE3_PKG_CFLAGS_OTHER)
        SET (SQLITE3_CFLAGS_OTHER "")
    ENDIF (SQLITE3_PKG_CFLAGS_OTHER)
    SET (SQLITE3_INCLUDE_DIRS ${SQLITE3_PKG_INCLUDE_DIRS})
    SET (SQLITE3_LINK_LIBS ${SQLITE3_PKG_LIBRARIES})
    SET (SQLITE3_LIBRARY_DIRS ${SQLITE3_PKG_LIBRARY_DIRS})
    IF (SQLITE3_PKG_LDFLAGS_OTHER)
        LIST_TO_STRING (SQLITE3_LINK_FLAGS ${SQLITE3_PKG_LDFLAGS_OTHER})
    ELSE (SQLITE3_PKG_LDFLAGS_OTHER)
        SET (SQLITE3_LINK_FLAGS "")
    ENDIF (SQLITE3_PKG_LDFLAGS_OTHER)
ENDIF (NOT PKG_CONFIG_FOUND)


