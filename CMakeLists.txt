cmake_minimum_required(VERSION 3.18)

option(ENABLE_VCPKG "Enable vcpkg for dependency management" ON)
set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg")
if(ENABLE_VCPKG AND EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
elseif(NOT ENABLE_VCPKG)
    # If VCPKG is disabled, ensure toolchain file is not set to a relative path from presets
    if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake$")
        unset(CMAKE_TOOLCHAIN_FILE CACHE)
        unset(CMAKE_TOOLCHAIN_FILE)
    endif()
endif()

project(flamerobin)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Add maximum debug information to Debug builds for better debugging.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Don't allow in-source builds. Keeps things nice and tidy
if( "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
   message( FATAL_ERROR
      "In-source builds not allowed.\n"
      "Create a new directory and run cmake from there, i.e.:\n"
      "  mkdir build\n"
      "  cd build\n"
      "  cmake ..\n"
      "You will need to delete CMakeCache.txt and CMakeFiles from this directory to clean up."
   )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#--------------------------------------
# fb-cpp location
set(firebird_DIR "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "Path to firebirdConfig.cmake")
set(FB_CPP_SKIP_INSTALL ON CACHE BOOL "Skip fb-cpp installation")
set(FB_CPP_BUILD_TESTS OFF CACHE BOOL "Build fb-cpp tests")
option(FB_CPP_USE_BOOST_DLL "Enable Boost.DLL support for loading fbclient at runtime" OFF)
option(FB_CPP_USE_BOOST_MULTIPRECISION "Enable Boost.Multiprecision helpers for INT128 and DECFLOAT types" OFF)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/fb-cpp/CMakeLists.txt")
    option(ENABLE_INTERNAL_FBCPP "Build fb-cpp from internal submodule" ON)
else()
    option(ENABLE_INTERNAL_FBCPP "Build fb-cpp from internal submodule" OFF)
endif()

if(ENABLE_INTERNAL_FBCPP)
    message(STATUS "Building fb-cpp internally from submodule")
    set(OLD_BUILD_TESTING ${BUILD_TESTING})
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    add_subdirectory(src/fb-cpp EXCLUDE_FROM_ALL)
    set(BUILD_TESTING ${OLD_BUILD_TESTING} CACHE BOOL "" FORCE)
else()
    message(STATUS "Searching for system fb-cpp package")
    find_package(fb-cpp CONFIG REQUIRED)
endif()

if(ENABLE_INTERNAL_FBCPP)
    set(FBCPP_VERSION_STR "${fb-cpp-main_VERSION}")
else()
    set(FBCPP_VERSION_STR "${fb-cpp_VERSION}")
endif()

if (NOT FBCPP_VERSION_STR)
    set(FBCPP_VERSION_STR "unknown")
endif()

#--------------------------------------
# wxWidgets location

if (WIN32)
	# On Windows we want the static build of wxWidgets
	set(wxWidgets_USE_STATIC ON)
endif (WIN32)

# Unicode support is always required
set(wxWidgets_USE_UNICODE ON)

# Find the wxWidgets library
# Try CONFIG mode first (vcpkg provides this), then fall back to standard FindwxWidgets.cmake
find_package(wxWidgets CONFIG QUIET COMPONENTS base core html xml aui stc adv net)
if (wxWidgets_FOUND AND TARGET wx::base)
    message(STATUS "Found wxWidgets via CONFIG: ${wxWidgets_VERSION}")
    set(wxWidgets_LIBRARIES wx::base wx::core wx::html wx::xml wx::aui wx::stc wx::adv wx::net)
else()
    message(STATUS "wxWidgets CONFIG not found, trying standard search")
    if (WIN32)
        set(wxWidgets_USE_STATIC ON)
        find_package(wxWidgets REQUIRED core base html xml aui stc adv net)
    elseif (UNIX AND NOT APPLE)
        find_package(wxWidgets REQUIRED core base html xml aui stc adv net)
    else()
        find_package(wxWidgets REQUIRED core base html xml aui stc adv net)
    endif()
endif()

if (wxWidgets_FOUND)
    message(STATUS "wxWidgets version : ${wxWidgets_VERSION_STRING}")
    if (NOT TARGET wx::base)
        include(${wxWidgets_USE_FILE})
    endif()
endif()

#--------------------------------------
# Source files
set(SOURCEDIR src/) # Hold over from old build system, probably overkill, but doesn't hurt

# FlameRobin app source files
list(APPEND SOURCE_LIST
        ${SOURCEDIR}/addconstrainthandler.cpp
        ${SOURCEDIR}/databasehandler.cpp
        ${SOURCEDIR}/frprec.cpp
        ${SOURCEDIR}/frutils.cpp
        ${SOURCEDIR}/logger.cpp
        ${SOURCEDIR}/main.cpp
        ${SOURCEDIR}/MasterPassword.cpp
        ${SOURCEDIR}/objectdescriptionhandler.cpp
        ${SOURCEDIR}/statementHistory.cpp
        ${SOURCEDIR}/config/Config.cpp
        ${SOURCEDIR}/config/DatabaseConfig.cpp
        ${SOURCEDIR}/config/LocaleManager.cpp
        ${SOURCEDIR}/config/LocalSettings.cpp
        ${SOURCEDIR}/engine/db/DatabaseFactory.cpp
        ${SOURCEDIR}/engine/db/ibpp/IbppDatabase.cpp
        ${SOURCEDIR}/engine/db/ibpp/IbppTransaction.cpp
        ${SOURCEDIR}/engine/db/ibpp/IbppStatement.cpp
        ${SOURCEDIR}/engine/db/ibpp/IbppService.cpp
        ${SOURCEDIR}/engine/db/ibpp/IbppBlob.cpp
        ${SOURCEDIR}/engine/db/fbcpp/FbCppDatabase.cpp
        ${SOURCEDIR}/engine/db/fbcpp/FbCppTransaction.cpp
        ${SOURCEDIR}/engine/db/fbcpp/FbCppStatement.cpp
        ${SOURCEDIR}/engine/db/fbcpp/FbCppService.cpp
        ${SOURCEDIR}/engine/db/fbcpp/FbCppBlob.cpp
        ${SOURCEDIR}/core/ArtProvider.cpp
        ${SOURCEDIR}/core/CodeTemplateProcessor.cpp
        ${SOURCEDIR}/core/FRDecimal.cpp
        ${SOURCEDIR}/core/FRError.cpp
        ${SOURCEDIR}/core/FRInt128.cpp
        ${SOURCEDIR}/core/FirebirdTypeUtils.cpp
        ${SOURCEDIR}/core/Observer.cpp
        ${SOURCEDIR}/core/ProgressIndicator.cpp
        ${SOURCEDIR}/core/StringUtils.cpp
        ${SOURCEDIR}/core/Subject.cpp
        ${SOURCEDIR}/core/TemplateProcessor.cpp
        ${SOURCEDIR}/core/URIProcessor.cpp
        ${SOURCEDIR}/core/Visitor.cpp
        ${SOURCEDIR}/engine/MetadataLoader.cpp
        ${SOURCEDIR}/gui/AboutBox.cpp
        ${SOURCEDIR}/gui/AdvancedMessageDialog.cpp
        ${SOURCEDIR}/gui/AdvancedSearchFrame.cpp
        ${SOURCEDIR}/gui/BackupFrame.cpp
        ${SOURCEDIR}/gui/BackupRestoreBaseFrame.cpp
        ${SOURCEDIR}/gui/BaseDialog.cpp
        ${SOURCEDIR}/gui/BaseFrame.cpp
        ${SOURCEDIR}/gui/CommandManager.cpp
        ${SOURCEDIR}/gui/ConfdefTemplateProcessor.cpp
        ${SOURCEDIR}/gui/ContextMenuMetadataItemVisitor.cpp
        ${SOURCEDIR}/gui/CreateIndexDialog.cpp
        ${SOURCEDIR}/gui/DatabaseRegistrationDialog.cpp
        ${SOURCEDIR}/gui/DataGeneratorFrame.cpp
        ${SOURCEDIR}/gui/EditBlobDialog.cpp
        ${SOURCEDIR}/gui/EventWatcherFrame.cpp
        ${SOURCEDIR}/gui/ExecuteSql.cpp
        ${SOURCEDIR}/gui/ExecuteSqlFrame.cpp
        ${SOURCEDIR}/gui/FieldPropertiesDialog.cpp
        ${SOURCEDIR}/gui/FindDialog.cpp
        ${SOURCEDIR}/gui/FRLayoutConfig.cpp
        ${SOURCEDIR}/gui/FRStyle.cpp
        ${SOURCEDIR}/gui/FRStyleManager.cpp
        ${SOURCEDIR}/gui/GUIURIHandlerHelper.cpp
        ${SOURCEDIR}/gui/HtmlHeaderMetadataItemVisitor.cpp
        ${SOURCEDIR}/gui/HtmlTemplateProcessor.cpp
        ${SOURCEDIR}/gui/InsertDialog.cpp
        ${SOURCEDIR}/gui/InsertParametersDialog.cpp
        ${SOURCEDIR}/gui/MainFrame.cpp
        ${SOURCEDIR}/gui/MetadataItemPropertiesFrame.cpp
        ${SOURCEDIR}/gui/MultilineEnterDialog.cpp
        ${SOURCEDIR}/gui/PreferencesDialog.cpp
        ${SOURCEDIR}/gui/PreferencesDialogSettings.cpp
        ${SOURCEDIR}/gui/PreferencesDialogStyle.cpp
        ${SOURCEDIR}/gui/PrivilegesDialog.cpp
        ${SOURCEDIR}/gui/ProgressDialog.cpp
        ${SOURCEDIR}/gui/ReorderFieldsDialog.cpp
        ${SOURCEDIR}/gui/RestoreFrame.cpp
        ${SOURCEDIR}/gui/ServerRegistrationDialog.cpp
        ${SOURCEDIR}/gui/ServiceBaseFrame.cpp
        ${SOURCEDIR}/gui/ShortcutCustomizationDialog.cpp
        ${SOURCEDIR}/gui/ShutdownFrame.cpp
        ${SOURCEDIR}/gui/ShutdownStartupBaseFrame.cpp
        ${SOURCEDIR}/gui/SimpleHtmlFrame.cpp
        ${SOURCEDIR}/gui/StartupFrame.cpp
        ${SOURCEDIR}/gui/StatementHistoryDialog.cpp
        ${SOURCEDIR}/gui/StyleGuide.cpp
        ${SOURCEDIR}/gui/UpdateChecker.cpp
        ${SOURCEDIR}/gui/UserDialog.cpp
        ${SOURCEDIR}/gui/UsernamePasswordDialog.cpp
        ${SOURCEDIR}/gui/controls/ControlUtils.cpp
        ${SOURCEDIR}/gui/controls/DataGrid.cpp
        ${SOURCEDIR}/gui/controls/DataGridRowBuffer.cpp
        ${SOURCEDIR}/gui/controls/DataGridRows.cpp
        ${SOURCEDIR}/gui/controls/DataGridTable.cpp
        ${SOURCEDIR}/gui/controls/DBHTreeControl.cpp
        ${SOURCEDIR}/gui/controls/DndTextControls.cpp
        ${SOURCEDIR}/gui/controls/LogTextControl.cpp
        ${SOURCEDIR}/gui/controls/PrintableHtmlWindow.cpp
        ${SOURCEDIR}/gui/controls/TextControl.cpp
        ${SOURCEDIR}/metadata/CharacterSet.cpp
        ${SOURCEDIR}/metadata/Collation.cpp
        ${SOURCEDIR}/metadata/column.cpp
        ${SOURCEDIR}/metadata/constraints.cpp
        ${SOURCEDIR}/metadata/CreateDDLVisitor.cpp
        ${SOURCEDIR}/metadata/database.cpp
        ${SOURCEDIR}/metadata/domain.cpp
        ${SOURCEDIR}/metadata/exception.cpp
        ${SOURCEDIR}/metadata/function.cpp
        ${SOURCEDIR}/metadata/generator.cpp
        ${SOURCEDIR}/metadata/Index.cpp
        ${SOURCEDIR}/metadata/IndexDDL.cpp
        ${SOURCEDIR}/metadata/metadataitem.cpp
        ${SOURCEDIR}/metadata/MetadataItemCreateStatementVisitor.cpp
        ${SOURCEDIR}/metadata/MetadataItemDescriptionVisitor.cpp
        ${SOURCEDIR}/metadata/MetadataItemURIHandlerHelper.cpp
        ${SOURCEDIR}/metadata/MetadataItemVisitor.cpp
        ${SOURCEDIR}/metadata/MetadataTemplateCmdHandler.cpp
        ${SOURCEDIR}/metadata/MetadataTemplateManager.cpp
        ${SOURCEDIR}/metadata/package.cpp
        ${SOURCEDIR}/metadata/parameter.cpp
        ${SOURCEDIR}/metadata/privilege.cpp
        ${SOURCEDIR}/metadata/procedure.cpp
        ${SOURCEDIR}/metadata/relation.cpp
        ${SOURCEDIR}/metadata/role.cpp
        ${SOURCEDIR}/metadata/root.cpp
        ${SOURCEDIR}/metadata/server.cpp
        ${SOURCEDIR}/metadata/table.cpp
        ${SOURCEDIR}/metadata/trigger.cpp
        ${SOURCEDIR}/metadata/User.cpp
        ${SOURCEDIR}/metadata/view.cpp
        ${SOURCEDIR}/sql/Identifier.cpp
        ${SOURCEDIR}/sql/IncompleteStatement.cpp
        ${SOURCEDIR}/sql/MultiStatement.cpp
        ${SOURCEDIR}/sql/SelectStatement.cpp
        ${SOURCEDIR}/sql/SqlStatement.cpp
        ${SOURCEDIR}/sql/SqlTokenizer.cpp
        ${SOURCEDIR}/sql/StatementBuilder.cpp
)
list(APPEND HEADER_LIST
        ${SOURCEDIR}/frutils.h
        ${SOURCEDIR}/frversion.h
        ${SOURCEDIR}/Isaac.h
        ${SOURCEDIR}/logger.h
        ${SOURCEDIR}/main.h
        ${SOURCEDIR}/MasterPassword.h
        ${SOURCEDIR}/revisioninfo.h
        ${SOURCEDIR}/statementHistory.h
        ${SOURCEDIR}/config/Config.h
        ${SOURCEDIR}/config/DatabaseConfig.h
        ${SOURCEDIR}/config/LocaleManager.h
        ${SOURCEDIR}/config/LocalSettings.h
        ${SOURCEDIR}/engine/db/DatabaseBackend.h
        ${SOURCEDIR}/engine/db/IDatabase.h
        ${SOURCEDIR}/engine/db/ITransaction.h
        ${SOURCEDIR}/engine/db/IStatement.h
        ${SOURCEDIR}/engine/db/IService.h
        ${SOURCEDIR}/engine/db/DatabaseFactory.h
        ${SOURCEDIR}/engine/db/ibpp/IbppDatabase.h
        ${SOURCEDIR}/engine/db/ibpp/IbppTransaction.h
        ${SOURCEDIR}/engine/db/ibpp/IbppStatement.h
        ${SOURCEDIR}/engine/db/ibpp/IbppService.h
        ${SOURCEDIR}/engine/db/fbcpp/FbCppDatabase.h
        ${SOURCEDIR}/engine/db/fbcpp/FbCppTransaction.h
        ${SOURCEDIR}/engine/db/fbcpp/FbCppStatement.h
        ${SOURCEDIR}/core/ArtProvider.h
        ${SOURCEDIR}/core/CodeTemplateProcessor.h
        ${SOURCEDIR}/core/FRDecimal.h
        ${SOURCEDIR}/core/FRError.h
        ${SOURCEDIR}/core/FRInt128.h
        ${SOURCEDIR}/core/ObjectWithHandle.h
        ${SOURCEDIR}/core/Observer.h
        ${SOURCEDIR}/core/ProcessableObject.h
        ${SOURCEDIR}/core/ProgressIndicator.h
        ${SOURCEDIR}/core/StringUtils.h
        ${SOURCEDIR}/core/Subject.h
        ${SOURCEDIR}/core/TemplateProcessor.h
        ${SOURCEDIR}/core/URIProcessor.h
        ${SOURCEDIR}/core/Visitor.h
        ${SOURCEDIR}/engine/MetadataLoader.h
        ${SOURCEDIR}/gui/AboutBox.h
        ${SOURCEDIR}/gui/AdvancedMessageDialog.h
        ${SOURCEDIR}/gui/AdvancedSearchFrame.h
        ${SOURCEDIR}/gui/BackupFrame.h
        ${SOURCEDIR}/gui/BackupRestoreBaseFrame.h
        ${SOURCEDIR}/gui/BaseDialog.h
        ${SOURCEDIR}/gui/BaseFrame.h
        ${SOURCEDIR}/gui/CommandIds.h
        ${SOURCEDIR}/gui/CommandManager.h
        ${SOURCEDIR}/gui/ConfdefTemplateProcessor.h
        ${SOURCEDIR}/gui/ContextMenuMetadataItemVisitor.h
        ${SOURCEDIR}/gui/CreateIndexDialog.h
        ${SOURCEDIR}/gui/DatabaseRegistrationDialog.h
        ${SOURCEDIR}/gui/DataGeneratorFrame.h
        ${SOURCEDIR}/gui/EditBlobDialog.h
        ${SOURCEDIR}/gui/EventWatcherFrame.h
        ${SOURCEDIR}/gui/ExecuteSql.h
        ${SOURCEDIR}/gui/ExecuteSqlFrame.h
        ${SOURCEDIR}/gui/FieldPropertiesDialog.h
        ${SOURCEDIR}/gui/FindDialog.h
        ${SOURCEDIR}/gui/FRLayoutConfig.h
        ${SOURCEDIR}/gui/FRStyle.h
        ${SOURCEDIR}/gui/FRStyleManager.h
        ${SOURCEDIR}/gui/GUIURIHandlerHelper.h
        ${SOURCEDIR}/gui/HtmlHeaderMetadataItemVisitor.h
        ${SOURCEDIR}/gui/HtmlTemplateProcessor.h
        ${SOURCEDIR}/gui/InsertDialog.h
        ${SOURCEDIR}/gui/InsertParametersDialog.h
        ${SOURCEDIR}/gui/MainFrame.h
        ${SOURCEDIR}/gui/MetadataItemPropertiesFrame.h
        ${SOURCEDIR}/gui/MultilineEnterDialog.h
        ${SOURCEDIR}/gui/PreferencesDialog.h
        ${SOURCEDIR}/gui/PreferencesDialogStyle.h
        ${SOURCEDIR}/gui/PrivilegesDialog.h
        ${SOURCEDIR}/gui/ProgressDialog.h
        ${SOURCEDIR}/gui/ReorderFieldsDialog.h
        ${SOURCEDIR}/gui/RestoreFrame.h
        ${SOURCEDIR}/gui/ServerRegistrationDialog.h
        ${SOURCEDIR}/gui/ServiceBaseFrame.h
        ${SOURCEDIR}/gui/ShortcutCustomizationDialog.h
        ${SOURCEDIR}/gui/ShutdownFrame.h
        ${SOURCEDIR}/gui/ShutdownStartupBaseFrame.h
        ${SOURCEDIR}/gui/SimpleHtmlFrame.h
        ${SOURCEDIR}/gui/StartupFrame.h
        ${SOURCEDIR}/gui/StatementHistoryDialog.h
        ${SOURCEDIR}/gui/StyleGuide.h
        ${SOURCEDIR}/gui/UpdateChecker.h
        ${SOURCEDIR}/gui/UserDialog.h
        ${SOURCEDIR}/gui/UsernamePasswordDialog.h
        ${SOURCEDIR}/gui/controls/ControlUtils.h
        ${SOURCEDIR}/gui/controls/DataGrid.h
        ${SOURCEDIR}/gui/controls/DataGridRowBuffer.h
        ${SOURCEDIR}/gui/controls/DataGridRows.h
        ${SOURCEDIR}/gui/controls/DataGridTable.h
        ${SOURCEDIR}/gui/controls/DBHTreeControl.h
        ${SOURCEDIR}/gui/controls/DndTextControls.h
        ${SOURCEDIR}/gui/controls/LogTextControl.h
        ${SOURCEDIR}/gui/controls/PrintableHtmlWindow.h
        ${SOURCEDIR}/gui/controls/TextControl.h
        ${SOURCEDIR}/metadata/CharacterSet.h
        ${SOURCEDIR}/metadata/Collation.h
        ${SOURCEDIR}/metadata/collection.h
        ${SOURCEDIR}/metadata/column.h
        ${SOURCEDIR}/metadata/constraints.h
        ${SOURCEDIR}/metadata/CreateDDLVisitor.h
        ${SOURCEDIR}/metadata/database.h
        ${SOURCEDIR}/metadata/domain.h
        ${SOURCEDIR}/metadata/exception.h
        ${SOURCEDIR}/metadata/function.h
        ${SOURCEDIR}/metadata/generator.h
        ${SOURCEDIR}/metadata/Index.h
        ${SOURCEDIR}/metadata/IndexDDL.h
        ${SOURCEDIR}/metadata/MetadataClasses.h
        ${SOURCEDIR}/metadata/metadataitem.h
        ${SOURCEDIR}/metadata/MetadataItemCreateStatementVisitor.h
        ${SOURCEDIR}/metadata/MetadataItemDescriptionVisitor.h
        ${SOURCEDIR}/metadata/MetadataItemURIHandlerHelper.h
        ${SOURCEDIR}/metadata/MetadataItemVisitor.h
        ${SOURCEDIR}/metadata/MetadataTemplateManager.h
        ${SOURCEDIR}/metadata/package.h
        ${SOURCEDIR}/metadata/parameter.h
        ${SOURCEDIR}/metadata/privilege.h
        ${SOURCEDIR}/metadata/procedure.h
        ${SOURCEDIR}/metadata/relation.h
        ${SOURCEDIR}/metadata/role.h
        ${SOURCEDIR}/metadata/root.h
        ${SOURCEDIR}/metadata/server.h
        ${SOURCEDIR}/metadata/table.h
        ${SOURCEDIR}/metadata/trigger.h
        ${SOURCEDIR}/metadata/User.h
        ${SOURCEDIR}/metadata/view.h
        ${SOURCEDIR}/sql/Identifier.h
        ${SOURCEDIR}/sql/IncompleteStatement.h
        ${SOURCEDIR}/sql/MultiStatement.h
        ${SOURCEDIR}/sql/SelectStatement.h
        ${SOURCEDIR}/sql/SqlStatement.h
        ${SOURCEDIR}/sql/SqlTokenizer.h
        ${SOURCEDIR}/sql/StatementBuilder.h
)

# IBPP static lib source files
list(APPEND IBPP_SOURCE_LIST
        ${SOURCEDIR}/ibpp/_dpb.cpp
        ${SOURCEDIR}/ibpp/_ibpp.cpp
        ${SOURCEDIR}/ibpp/_ibs.cpp
        ${SOURCEDIR}/ibpp/_rb.cpp
        ${SOURCEDIR}/ibpp/_spb.cpp
        ${SOURCEDIR}/ibpp/_tpb.cpp
        ${SOURCEDIR}/ibpp/array.cpp
        ${SOURCEDIR}/ibpp/blob.cpp
        ${SOURCEDIR}/ibpp/database.cpp
        ${SOURCEDIR}/ibpp/date.cpp
        ${SOURCEDIR}/ibpp/dbkey.cpp
        ${SOURCEDIR}/ibpp/events.cpp
        ${SOURCEDIR}/ibpp/exception.cpp
        ${SOURCEDIR}/ibpp/fbinterfaces.cpp
        ${SOURCEDIR}/ibpp/ibint128.cpp
        ${SOURCEDIR}/ibpp/row.cpp
        ${SOURCEDIR}/ibpp/service.cpp
        ${SOURCEDIR}/ibpp/statement.cpp
        ${SOURCEDIR}/ibpp/time.cpp
        ${SOURCEDIR}/ibpp/transaction.cpp
        ${SOURCEDIR}/ibpp/user.cpp
)
list(APPEND IBPP_HEADER_LIST
        ${SOURCEDIR}/firebird/include/ibase.h
        ${SOURCEDIR}/firebird/include/iberror.h
        ${SOURCEDIR}/firebird/include/firebird/impl/blr.h
        ${SOURCEDIR}/firebird/include/firebird/impl/consts_pub.h
        ${SOURCEDIR}/firebird/include/firebird/impl/dsc_pub.h
        ${SOURCEDIR}/firebird/include/firebird/impl/inf_pub.h
        ${SOURCEDIR}/firebird/include/firebird/impl/msg_helper.h
        ${SOURCEDIR}/firebird/include/firebird/impl/sqlda_pub.h
        ${SOURCEDIR}/firebird/include/firebird/impl/types_pub.h        
		${SOURCEDIR}/firebird/include/firebird/impl/msg/all.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/dsql.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/dyn.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/fbsvcmgr.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/fbtracemgr.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/gbak.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/gfix.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/gsec.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/gstat.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/isql.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/jrd.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/jrd_bugchk.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/nbackup.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/sqlerr.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/sqlwarn.h
		${SOURCEDIR}/firebird/include/firebird/impl/msg/utl.h		
		${SOURCEDIR}/firebird/include/firebird/Interface.h
		${SOURCEDIR}/firebird/include/firebird/IdlFbInterfaces.h
        ${SOURCEDIR}/ibpp/_ibpp.h
        ${SOURCEDIR}/ibpp/ibpp.h
)

#--------------------------------------
# Platform specific options

if (WIN32)
	# Windows specific details
	list(APPEND SOURCE_LIST
		${SOURCEDIR}/gui/msw/StyleGuideMSW.cpp
	)
	list(APPEND RESOURCE_LIST
		res/flamerobin.rc
	)
	add_definitions(-DWINVER=0x500 -DWIN32_LEAN_AND_MEAN -DIBPP_WINDOWS)
	remove_definitions(-DUNICODE -D_UNICODE) # IBPP doesn't like Unicode	

	if (MSVC)
		# Visual Studio specific stuff
	endif (MSVC)

	# Create the revision include file
	execute_process(
		COMMAND update-revision-info.cmd
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
    list(APPEND FR_LIBS ws2_32 mswsock shlwapi version advapi32 gdi32 user32 comctl32 comdlg32 shell32 ole32 oleaut32 uuid imm32 msimg32 winmm uxtheme rpcrt4 winspool)
    add_definitions(-DPCRE2_STATIC)

    # When using vcpkg and static linking, we need to explicitly link some wxWidgets dependencies
    # that are not always automatically pulled in by find_package(wxWidgets).
    include(SelectLibraryConfigurations)

    set(FR_VCPKG_LIB_HINT "")
    if (VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
        set(FR_VCPKG_LIB_HINT HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
        set(FR_VCPKG_DEBUG_LIB_HINT HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib")
    endif()

    # pcre2
    find_package(pcre2 CONFIG REQUIRED)
    if (TARGET pcre2::pcre2-16)
        message(STATUS "Using vcpkg pcre2::pcre2-16 target")
        list(APPEND FR_LIBS pcre2::pcre2-16)
    else()
        find_library(PCRE2_RELEASE NAMES pcre2-16 pcre2-16-static wxpcre2 wxpcre2-static ${FR_VCPKG_LIB_HINT})
        find_library(PCRE2_DEBUG NAMES pcre2-16d pcre2-16-staticd wxpcre2d wxpcre2-staticd ${FR_VCPKG_DEBUG_LIB_HINT})
        select_library_configurations(PCRE2)
        if (PCRE2_LIBRARY)
            message(STATUS "Found PCRE2 library: ${PCRE2_LIBRARY}")
            list(APPEND FR_LIBS ${PCRE2_LIBRARY})
        endif()
    endif()
    
    # Lexilla
    find_package(lexilla CONFIG QUIET)
    if (TARGET lexilla::lexilla)
        message(STATUS "Using vcpkg lexilla::lexilla target")
        list(APPEND FR_LIBS lexilla::lexilla)
    else()
        find_library(LEXILLA_RELEASE NAMES Lexilla lexilla wxlexilla wxlexilla-static ${FR_VCPKG_LIB_HINT})
        find_library(LEXILLA_DEBUG NAMES Lexillad lexillad wxlexillad wxlexillad-static ${FR_VCPKG_DEBUG_LIB_HINT})
        select_library_configurations(LEXILLA)
        if (LEXILLA_LIBRARY)
            message(STATUS "Found Lexilla library: ${LEXILLA_LIBRARY}")
            list(APPEND FR_LIBS ${LEXILLA_LIBRARY})
        endif()
    endif()

    # Scintilla
    find_package(scintilla CONFIG QUIET)
    if (TARGET scintilla::scintilla)
        message(STATUS "Using vcpkg scintilla::scintilla target")
        list(APPEND FR_LIBS scintilla::scintilla)
    else()
        find_library(SCINTILLA_RELEASE NAMES Scintilla scintilla wxscintilla wxscintilla-static ${FR_VCPKG_LIB_HINT})
        find_library(SCINTILLA_DEBUG NAMES Scintillad scintillad wxscintillad wxscintillad-static ${FR_VCPKG_DEBUG_LIB_HINT})
        select_library_configurations(SCINTILLA)
        if (SCINTILLA_LIBRARY)
            message(STATUS "Found Scintilla library: ${SCINTILLA_LIBRARY}")
            list(APPEND FR_LIBS ${SCINTILLA_LIBRARY})
        endif()
    endif()
    
    message(STATUS "Final FR_LIBS on Windows: ${FR_LIBS}")
endif (WIN32)

if (UNIX AND NOT APPLE)
	# UNIX (Linux) specific details
	list(APPEND SOURCE_LIST
		${SOURCEDIR}/gui/gtk/StyleGuideGTK.cpp
	)
	add_definitions(-DIBPP_LINUX)
	add_definitions(-DFR_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

	find_package(firebird REQUIRED)
	# Need to link the fbclient lib in *nix systems
	list(APPEND FR_LIBS firebird::firebird -ldl)

	include(CheckCXXSourceCompiles)
	check_cxx_source_compiles("
int main() {
    __int128 x;
    return 0;
}
" HAVE_INT128)

	if (HAVE_INT128)
		add_definitions(-DHAVE_INT128)
	endif()

	check_cxx_source_compiles("
#include <decimal/decimal>
int main() {
    std::decimal::decimal128 x;
    return 0;
}
" HAVE_DECIMAL128)

	if (HAVE_DECIMAL128)
		add_definitions(-DHAVE_DECIMAL128)
	endif()

	# Create the revision include file
	execute_process(
		COMMAND "${CMAKE_SOURCE_DIR}/update-revision-info.sh"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)

	# Util script to help run FR in build dir
	file(COPY utils/run_flamerobin.sh  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif (UNIX AND NOT APPLE)

if(APPLE)
	# MacOS specific stuff
	# TODO: Untested, if any Mac devs want to check, please get in touch
	message(WARNING "MacOS build is untested.\n"
	                "Please get in contact to tell us of your results.")
	add_definitions(-DIBPP_DARWIN)
        set(FR_FIREBIRD_HINT_DIRS
                "/Library/Frameworks/Firebird.framework/Resources/lib"
                "/Library/Frameworks/Firebird.framework/Libraries"
                "/opt/homebrew/lib"
                "/usr/local/lib"
        )
        find_library(FBCLIENT_LIBRARY
                NAMES fbclient libfbclient
                HINTS ${FR_FIREBIRD_HINT_DIRS}
        )
        if (FBCLIENT_LIBRARY)
                message(STATUS "Using Firebird client library: ${FBCLIENT_LIBRARY}")
                list(APPEND FR_LIBS ${FBCLIENT_LIBRARY})
                get_filename_component(FBCLIENT_LIBRARY_DIR ${FBCLIENT_LIBRARY} DIRECTORY)
                list(APPEND FR_RPATHS ${FBCLIENT_LIBRARY_DIR})
        else ()
                message(FATAL_ERROR "Could not find libfbclient on macOS. Set CMAKE_PREFIX_PATH or FBLIB to a valid Firebird client location.")
        endif ()
        
    list(APPEND SOURCE_LIST
        ${SOURCEDIR}/gui/mac/StyleGuideMAC.cpp
    )

    add_definitions(-DFR_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
 
    # Create the revision include file
    execute_process(
        COMMAND "${CMAKE_SOURCE_DIR}/update-revision-info.sh"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    # Util script to help run FR in build dir
    file(COPY utils/run_flamerobin.sh  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    # Helper function for adding resource files
    function(add_resources VAR SOURCE_PATH DESTINATION PATTERN)
            file(GLOB_RECURSE _LIST CONFIGURE_DEPENDS ${SOURCE_PATH}/${PATTERN})
            foreach (RESOURCE ${_LIST})
                    get_filename_component(_PARENT ${RESOURCE} DIRECTORY)
                    if (${_PARENT} STREQUAL ${SOURCE_PATH})
                    set(_DESTINATION ${DESTINATION})
                    else ()
                    file(RELATIVE_PATH _DESTINATION ${SOURCE_PATH} ${_PARENT})
                    set(_DESTINATION ${DESTINATION}/${_DESTINATION})
                    endif ()
                    set_property(SOURCE ${RESOURCE} PROPERTY MACOSX_PACKAGE_LOCATION ${_DESTINATION})
            endforeach (RESOURCE)
            set(${VAR} ${_LIST} PARENT_SCOPE)
    endfunction()

    # Add resources
    add_resources(HTML_TEMPLATES ${CMAKE_SOURCE_DIR}/html-templates SharedSupport/html-templates *)
    add_resources(CONF_DEFS ${CMAKE_SOURCE_DIR}/conf-defs SharedSupport/conf-defs *)
    add_resources(DOCS ${CMAKE_SOURCE_DIR}/docs SharedSupport/docs *)
    add_resources(SYS_TEMPLATES ${CMAKE_SOURCE_DIR}/sys-templates SharedSupport/sys-templates *)
    add_resources(CODE_TEMPLATES ${CMAKE_SOURCE_DIR}/code-templates SharedSupport/code-templates *)
    add_resources(ICON ${CMAKE_SOURCE_DIR}/res Resources flamerobin.icns)
    add_resources(XML_STYLES ${CMAKE_SOURCE_DIR}/xml-styles SharedSupport/xml-styles *)

    set(RESOURCE_FILES_MAC
            ${HTML_TEMPLATES}
            ${CONF_DEFS}
            ${DOCS}
            ${SYS_TEMPLATES}
            ${CODE_TEMPLATES}
            ${ICON}
			${XML_STYLES}
    )

endif (APPLE)

# Organize Visual Studio projects (possibly others so it's outside the WIN32 section)
source_group("Source Files" FILES ${SOURCE_LIST})
source_group("Header Files" FILES ${HEADER_LIST})
source_group("Resource Files" FILES ${RESOURCE_LIST})
source_group("Source Files" FILES ${IBPP_SOURCE_LIST})
source_group("Header Files" FILES ${IBPP_HEADER_LIST})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT flamerobin)

#--------------------------------------
# IBPP lib
add_library(IBPP STATIC ${IBPP_SOURCE_LIST} ${IBPP_HEADER_LIST})

#--------------------------------------
# FlameRobin app
include_directories(BEFORE src src/ibpp src/firebird/include res)

if (NOT APPLE)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCE_LIST} ${HEADER_LIST} ${RESOURCE_LIST})
endif (NOT APPLE)

#--------------------------------------
# Copy template files to build location so we can run in debugger
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/html-templates DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/code-templates DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sys-templates  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/conf-defs  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/xml-styles  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if (APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCE_LIST} ${HEADER_LIST} ${RESOURCE_FILES_MAC})

    string(TIMESTAMP CURYEAR "%Y")
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "FlameRobin"
            MACOSX_BUNDLE_BUNDLE_IDENTIFIER "org.flamerobin"
            MACOSX_BUNDLE_COPYRIGHT "Copyright 2004-${CURYEAR} FlameRobin Team"
            MACOSX_BUNDLE_ICON_FILE "flamerobin.icns"
            MACOSX_BUNDLE_INFO_STRING "FlameRobin, Copyright 2004-${CURYEAR} FlameRobin Team"
    )
endif (APPLE)

target_link_libraries(${PROJECT_NAME} IBPP fb-cpp ${wxWidgets_LIBRARIES} ${FR_LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE FBCPP_VERSION="${FBCPP_VERSION_STR}")

enable_testing()
add_executable(index_ddl_test
    ${SOURCEDIR}/metadata/IndexDDLTest.cpp
    ${SOURCEDIR}/metadata/IndexDDL.cpp
)
target_link_libraries(index_ddl_test ${wxWidgets_LIBRARIES})
add_test(NAME index_ddl_test COMMAND index_ddl_test)

# Common source files shared by the SQL parsing tests.
# TestStringUtils.cpp provides a minimal wx2std() stub so that FRError.cpp
# links without dragging in the full Database / IBPP symbol chain.
set(SQL_TEST_COMMON_SOURCES
    ${SOURCEDIR}/sql/SqlTokenizer.cpp
    ${SOURCEDIR}/config/Config.cpp
    ${SOURCEDIR}/core/Observer.cpp
    ${SOURCEDIR}/core/Subject.cpp
    ${SOURCEDIR}/core/FRError.cpp
    ${SOURCEDIR}/sql/TestStringUtils.cpp
)

add_executable(multi_statement_test
    ${SOURCEDIR}/sql/MultiStatementTest.cpp
    ${SOURCEDIR}/sql/MultiStatement.cpp
    ${SQL_TEST_COMMON_SOURCES}
)
target_link_libraries(multi_statement_test ${wxWidgets_LIBRARIES})
add_test(NAME multi_statement_test COMMAND multi_statement_test)

add_executable(sql_tokenizer_test
    ${SOURCEDIR}/sql/SqlTokenizerTest.cpp
    ${SQL_TEST_COMMON_SOURCES}
)
target_link_libraries(sql_tokenizer_test ${wxWidgets_LIBRARIES})
add_test(NAME sql_tokenizer_test COMMAND sql_tokenizer_test)

add_executable(ibpp_row_metadata_test
    ${SOURCEDIR}/ibpp/RowMetadataTest.cpp
)
target_link_libraries(ibpp_row_metadata_test IBPP ${FR_LIBS})
add_test(NAME ibpp_row_metadata_test COMMAND ibpp_row_metadata_test)

add_executable(dal_types_test
    ${SOURCEDIR}/engine/db/DalTypesTest.cpp
    ${SOURCEDIR}/engine/db/DatabaseFactory.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppDatabase.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppTransaction.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppStatement.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppService.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppBlob.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppDatabase.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppTransaction.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppStatement.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppService.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppBlob.cpp
    ${SOURCEDIR}/core/FRError.cpp
    ${SOURCEDIR}/core/FRInt128.cpp
    ${SOURCEDIR}/core/FRDecimal.cpp
    ${SOURCEDIR}/core/StringUtils.cpp
)
target_link_libraries(dal_types_test IBPP fb-cpp ${wxWidgets_LIBRARIES} ${FR_LIBS})
add_test(NAME dal_types_test COMMAND dal_types_test)

# Regression test for issue #78 (EXECUTE BLOCK / Selectable SP emitting
# internal RDB$xxx domain names instead of actual datatypes).
# Tests the system-domain classification predicate (name prefix check) that
# drives Parameter::getTypeOf() and Procedure::getAlterSql().
add_executable(domain_is_system_test
    ${SOURCEDIR}/metadata/DomainIsSystemTest.cpp
)
target_link_libraries(domain_is_system_test ${wxWidgets_LIBRARIES})
add_test(NAME domain_is_system_test COMMAND domain_is_system_test)

# Regression test for issue #436 (fatal crash when dropping a trigger).
# Tests the Observer/Subject subjectRemoved() contract: observers must
# receive subjectRemoved() when a Subject is destroyed so they can clear
# any raw pointer they hold to it.
add_executable(subject_observer_test
    ${SOURCEDIR}/core/SubjectObserverTest.cpp
    ${SOURCEDIR}/core/Observer.cpp
    ${SOURCEDIR}/core/Subject.cpp
)
target_link_libraries(subject_observer_test ${wxWidgets_LIBRARIES})
add_test(NAME subject_observer_test COMMAND subject_observer_test)

add_executable(command_manager_test
    ${SOURCEDIR}/gui/CommandManagerTest.cpp
    ${SOURCEDIR}/gui/CommandManager.cpp
    ${SOURCEDIR}/config/Config.cpp
    ${SOURCEDIR}/core/Observer.cpp
    ${SOURCEDIR}/core/Subject.cpp
    ${SOURCEDIR}/core/FRError.cpp
    ${SOURCEDIR}/sql/TestStringUtils.cpp
)
target_link_libraries(command_manager_test ${wxWidgets_LIBRARIES})
add_test(NAME command_manager_test COMMAND command_manager_test)

add_executable(fbcpp_service_test
    ${SOURCEDIR}/engine/db/fbcpp/FbCppServiceTest.cpp
    ${SOURCEDIR}/engine/db/DatabaseFactory.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppDatabase.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppTransaction.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppStatement.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppService.cpp
    ${SOURCEDIR}/engine/db/ibpp/IbppBlob.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppDatabase.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppTransaction.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppStatement.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppService.cpp
    ${SOURCEDIR}/engine/db/fbcpp/FbCppBlob.cpp
    ${SOURCEDIR}/core/FRError.cpp
    ${SOURCEDIR}/core/FRInt128.cpp
    ${SOURCEDIR}/core/FRDecimal.cpp
    ${SOURCEDIR}/core/StringUtils.cpp
)
target_link_libraries(fbcpp_service_test IBPP fb-cpp ${wxWidgets_LIBRARIES} ${FR_LIBS})
add_test(NAME fbcpp_service_test COMMAND fbcpp_service_test)

if (APPLE)
        set_target_properties(${PROJECT_NAME} PROPERTIES
                BUILD_WITH_INSTALL_RPATH FALSE
                BUILD_RPATH "${FR_RPATHS}"
                INSTALL_RPATH "${FR_RPATHS}"
                MACOSX_RPATH TRUE
        )
endif (APPLE)


#--------------------------------------
# Install
if (UNIX AND NOT APPLE)
	install(
		TARGETS flamerobin
		RUNTIME DESTINATION bin
	)
	install(
		DIRECTORY
			html-templates
			code-templates
			sys-templates
			conf-defs 
			xml-styles			
		DESTINATION share/flamerobin
	)
	install(
		FILES res/flamerobin.png
		DESTINATION share/pixmaps
	)
	install(
		FILES res/flamerobin.desktop
		DESTINATION share/applications
	)
	install(
		DIRECTORY
			docs
		DESTINATION share/flamerobin
		FILES_MATCHING
		PATTERN "*.html"
		PATTERN "*.css"
	)
	install(
		FILES docs/flamerobin.1
		DESTINATION share/man/man1
	)
endif (UNIX AND NOT APPLE)

if (APPLE)
	install(
		TARGETS flamerobin
		BUNDLE DESTINATION /Applications
		RUNTIME DESTINATION bin
	)
endif (APPLE)
