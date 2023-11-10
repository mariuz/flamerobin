cmake_minimum_required(VERSION 3.5)
project(flamerobin)

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

set(CMAKE_CXX_STANDARD 14)

#--------------------------------------
# wxWidgets location

# Attempt to get wxWidgets root dir from environment, or user
# From command line you can:
#   cmake -DWXDIR:PATH=C:\path\to\wxWidgets ..
if (WIN32)
	message(STATUS "Checking for user suppllied wxWidgets location (WXDIR)...")
	if(DEFINED WXDIR)
		message(STATUS "User set the WXDIR property directly : ${WXDIR}")
	else ()
		if(DEFINED ENV{WXDIR})
			message(STATUS "Using the WXDIR environment variable : $ENV{WXDIR}")
			set(WXDIR $ENV{WXDIR} CACHE PATH "wxWidgits root dir")
		else ()
			message(WARNING "WXDIR has not been set!")
		endif ()
	endif()
	set(wxWidgets_ROOT_DIR ${WXDIR})
endif (WIN32)

# We want the wx static build, and Unicode support
set(wxWidgets_USE_STATIC ON)
set(wxWidgets_USE_UNICODE ON)

# Find the wxWidgets library
find_package(wxWidgets REQUIRED core base html xml aui stc adv OPTIONAL_COMPONENTS scintilla)
message(STATUS "wxWidgets version : ${wxWidgets_VERSION_STRING}")

# For wx3.0 the Scintilla lib is not present, but older wx find_package doesn't seem to
# understand this (even if marked optional), so we have to recheck, without Scintilla
if ( (${CMAKE_VERSION} VERSION_LESS "3.16") AND (${wxWidgets_VERSION_STRING} VERSION_LESS "3.1") )
	message(STATUS "Re-doing find_package for wx3.0 builds...")
	find_package(wxWidgets REQUIRED core base html xml aui stc adv)
endif ()

# Add wxWidgets to include path
include(${wxWidgets_USE_FILE})

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
        ${SOURCEDIR}/config/LocalSettings.cpp
        ${SOURCEDIR}/core/ArtProvider.cpp
        ${SOURCEDIR}/core/CodeTemplateProcessor.cpp
        ${SOURCEDIR}/core/FRDecimal.cpp
        ${SOURCEDIR}/core/FRError.cpp
        ${SOURCEDIR}/core/FRInt128.cpp
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
        ${SOURCEDIR}/gui/PrivilegesDialog.cpp
        ${SOURCEDIR}/gui/ProgressDialog.cpp
        ${SOURCEDIR}/gui/ReorderFieldsDialog.cpp
        ${SOURCEDIR}/gui/RestoreFrame.cpp
        ${SOURCEDIR}/gui/ServerRegistrationDialog.cpp
        ${SOURCEDIR}/gui/ServiceBaseFrame.cpp
        ${SOURCEDIR}/gui/ShutdownFrame.cpp
        ${SOURCEDIR}/gui/ShutdownStartupBaseFrame.cpp
        ${SOURCEDIR}/gui/SimpleHtmlFrame.cpp
        ${SOURCEDIR}/gui/StartupFrame.cpp
        ${SOURCEDIR}/gui/StatementHistoryDialog.cpp
        ${SOURCEDIR}/gui/StyleGuide.cpp
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
        ${SOURCEDIR}/config/LocalSettings.h
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
        ${SOURCEDIR}/gui/GUIURIHandlerHelper.h
        ${SOURCEDIR}/gui/HtmlHeaderMetadataItemVisitor.h
        ${SOURCEDIR}/gui/HtmlTemplateProcessor.h
        ${SOURCEDIR}/gui/InsertDialog.h
        ${SOURCEDIR}/gui/InsertParametersDialog.h
        ${SOURCEDIR}/gui/MainFrame.h
        ${SOURCEDIR}/gui/MetadataItemPropertiesFrame.h
        ${SOURCEDIR}/gui/MultilineEnterDialog.h
        ${SOURCEDIR}/gui/PreferencesDialog.h
        ${SOURCEDIR}/gui/PrivilegesDialog.h
        ${SOURCEDIR}/gui/ProgressDialog.h
        ${SOURCEDIR}/gui/ReorderFieldsDialog.h
        ${SOURCEDIR}/gui/RestoreFrame.h
        ${SOURCEDIR}/gui/ServerRegistrationDialog.h
        ${SOURCEDIR}/gui/ServiceBaseFrame.h
        ${SOURCEDIR}/gui/ShutdownFrame.h
        ${SOURCEDIR}/gui/ShutdownStartupBaseFrame.h
        ${SOURCEDIR}/gui/SimpleHtmlFrame.h
        ${SOURCEDIR}/gui/StartupFrame.h
        ${SOURCEDIR}/gui/StatementHistoryDialog.h
        ${SOURCEDIR}/gui/StyleGuide.h
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
		# Make sure we use static MSVC libs
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	endif (MSVC)

	# Create the revision include file
	execute_process(
		COMMAND update-revision-info.cmd
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
endif (WIN32)

if (UNIX AND NOT APPLE)
	# UNIX (Linux) specific details
	list(APPEND SOURCE_LIST
		${SOURCEDIR}/gui/gtk/StyleGuideGTK.cpp
	)
	add_definitions(-DIBPP_LINUX)
	add_definitions(-DFR_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
	# Need to link the fbclient lib in *nix systems
	list(APPEND FR_LIBS -lfbclient)

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
	list(APPEND FR_LIBS -lfbclient)
        list(APPEND FR_LIBS -L/Library/Frameworks/Firebird.framework/Versions/Current/Libraries)
        
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
include_directories(BEFORE src src/ibpp res)

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

target_link_libraries(${PROJECT_NAME} IBPP ${wxWidgets_LIBRARIES} ${FR_LIBS})


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
                DESTINATION /Applications
        )
endif (APPLE)

