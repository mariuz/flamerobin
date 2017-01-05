# generated automatically by aclocal 1.15 -*- Autoconf -*-

# Copyright (C) 1996-2014 Free Software Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

m4_ifndef([AC_CONFIG_MACRO_DIRS], [m4_defun([_AM_CONFIG_MACRO_DIRS], [])m4_defun([AC_CONFIG_MACRO_DIRS], [_AM_CONFIG_MACRO_DIRS($@)])])
AC_DEFUN([AC_BAKEFILE_CREATE_FILE_DLLAR_SH],
[
dnl ===================== dllar.sh begins here =====================
dnl    (Created by merge-scripts.py from dllar.sh
dnl     file do not edit here!)
D='$'
cat <<EOF >dllar.sh
#!/bin/sh
#
# dllar - a tool to build both a .dll and an .a file
# from a set of object (.o) files for EMX/OS2.
#
#  Written by Andrew Zabolotny, bit@freya.etu.ru
#  Ported to Unix like shell by Stefan Neis, Stefan.Neis@t-online.de
#
#  This script will accept a set of files on the command line.
#  All the public symbols from the .o files will be exported into
#  a .DEF file, then linker will be run (through gcc) against them to
#  build a shared library consisting of all given .o files. All libraries
#  (.a) will be first decompressed into component .o files then act as
#  described above. You can optionally give a description (-d "description")
#  which will be put into .DLL. To see the list of accepted options (as well
#  as command-line format) simply run this program without options. The .DLL
#  is built to be imported by name (there is no guarantee that new versions
#  of the library you build will have same ordinals for same symbols).
#
#  dllar is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  dllar is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with dllar; see the file COPYING.  If not, write to the Free
#  Software Foundation, 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.

# To successfuly run this program you will need:
#  - Current drive should have LFN support (HPFS, ext2, network, etc)
#    (Sometimes dllar generates filenames which won't fit 8.3 scheme)
#  - gcc
#    (used to build the .dll)
#  - emxexp
#    (used to create .def file from .o files)
#  - emximp
#    (used to create .a file from .def file)
#  - GNU text utilites (cat, sort, uniq)
#    used to process emxexp output
#  - GNU file utilities (mv, rm)
#  - GNU sed
#  - lxlite (optional, see flag below)
#    (used for general .dll cleanup)
#

flag_USE_LXLITE=1;

#
# helper functions
# basnam, variant of basename, which does _not_ remove the path, _iff_
#                              second argument (suffix to remove) is given
basnam(){
    case ${D}# in
    1)
        echo ${D}1 | sed 's/.*\\///' | sed 's/.*\\\\//'
        ;;
    2)
        echo ${D}1 | sed 's/'${D}2'${D}//'
        ;;
    *)
        echo "error in basnam ${D}*"
        exit 8
        ;;
    esac
}

# Cleanup temporary files and output
CleanUp() {
    cd ${D}curDir
    for i in ${D}inputFiles ; do
        case ${D}i in
        *!)
            rm -rf \`basnam ${D}i !\`
            ;;
        *)
            ;;
        esac
    done

    # Kill result in case of failure as there is just to many stupid make/nmake
    # things out there which doesn't do this.
    if @<:@ ${D}# -eq 0 @:>@; then
        rm -f ${D}arcFile ${D}arcFile2 ${D}defFile ${D}dllFile
    fi
}

# Print usage and exit script with rc=1.
PrintHelp() {
 echo 'Usage: dllar.sh @<:@-o@<:@utput@:>@ output_file@:>@ @<:@-i@<:@mport@:>@ importlib_name@:>@'
 echo '       @<:@-name-mangler-script script.sh@:>@'
 echo '       @<:@-d@<:@escription@:>@ "dll descrption"@:>@ @<:@-cc "CC"@:>@ @<:@-f@<:@lags@:>@ "CFLAGS"@:>@'
 echo '       @<:@-ord@<:@inals@:>@@:>@ -ex@<:@clude@:>@ "symbol(s)"'
 echo '       @<:@-libf@<:@lags@:>@ "{INIT|TERM}{GLOBAL|INSTANCE}"@:>@ @<:@-nocrt@<:@dll@:>@@:>@ @<:@-nolxl@<:@ite@:>@@:>@'
 echo '       @<:@*.o@:>@ @<:@*.a@:>@'
 echo '*> "output_file" should have no extension.'
 echo '   If it has the .o, .a or .dll extension, it is automatically removed.'
 echo '   The import library name is derived from this and is set to "name".a,'
 echo '   unless overridden by -import'
 echo '*> "importlib_name" should have no extension.'
 echo '   If it has the .o, or .a extension, it is automatically removed.'
 echo '   This name is used as the import library name and may be longer and'
 echo '   more descriptive than the DLL name which has to follow the old '
 echo '   8.3 convention of FAT.'
 echo '*> "script.sh may be given to override the output_file name by a'
 echo '   different name. It is mainly useful if the regular make process'
 echo '   of some package does not take into account OS/2 restriction of'
 echo '   DLL name lengths. It takes the importlib name as input and is'
 echo '   supposed to procude a shorter name as output. The script should'
 echo '   expect to get importlib_name without extension and should produce'
 echo '   a (max.) 8 letter name without extension.'
 echo '*> "cc" is used to use another GCC executable.   (default: gcc.exe)'
 echo '*> "flags" should be any set of valid GCC flags. (default: -s -Zcrtdll)'
 echo '   These flags will be put at the start of GCC command line.'
 echo '*> -ord@<:@inals@:>@ tells dllar to export entries by ordinals. Be careful.'
 echo '*> -ex@<:@clude@:>@ defines symbols which will not be exported. You can define'
 echo '   multiple symbols, for example -ex "myfunc yourfunc _GLOBAL*".'
 echo '   If the last character of a symbol is "*", all symbols beginning'
 echo '   with the prefix before "*" will be exclude, (see _GLOBAL* above).'
 echo '*> -libf@<:@lags@:>@ can be used to add INITGLOBAL/INITINSTANCE and/or'
 echo '   TERMGLOBAL/TERMINSTANCE flags to the dynamically-linked library.'
 echo '*> -nocrt@<:@dll@:>@ switch will disable linking the library against emx''s'
 echo '   C runtime DLLs.'
 echo '*> -nolxl@<:@ite@:>@ switch will disable running lxlite on the resulting DLL.'
 echo '*> All other switches (for example -L./ or -lmylib) will be passed'
 echo '   unchanged to GCC at the end of command line.'
 echo '*> If you create a DLL from a library and you do not specify -o,'
 echo '   the basename for DLL and import library will be set to library name,'
 echo '   the initial library will be renamed to 'name'_s.a (_s for static)'
 echo '   i.e. "dllar gcc.a" will create gcc.dll and gcc.a, and the initial'
 echo '   library will be renamed into gcc_s.a.'
 echo '--------'
 echo 'Example:'
 echo '   dllar -o gcc290.dll libgcc.a -d "GNU C runtime library" -ord'
 echo '    -ex "__main __ctordtor*" -libf "INITINSTANCE TERMINSTANCE"'
 CleanUp
 exit 1
}

# Execute a command.
# If exit code of the commnad <> 0 CleanUp() is called and we'll exit the script.
# @Uses    Whatever CleanUp() uses.
doCommand() {
    echo "${D}*"
    eval ${D}*
    rcCmd=${D}?

    if @<:@ ${D}rcCmd -ne 0 @:>@; then
        echo "command failed, exit code="${D}rcCmd
        CleanUp
        exit ${D}rcCmd
    fi
}

# main routine
# setup globals
cmdLine=${D}*
outFile=""
outimpFile=""
inputFiles=""
renameScript=""
description=""
CC=gcc.exe
CFLAGS="-s -Zcrtdll"
EXTRA_CFLAGS=""
EXPORT_BY_ORDINALS=0
exclude_symbols=""
library_flags=""
curDir=\`pwd\`
curDirS=curDir
case ${D}curDirS in
*/)
  ;;
*)
  curDirS=${D}{curDirS}"/"
  ;;
esac
# Parse commandline
libsToLink=0
omfLinking=0
while @<:@ ${D}1 @:>@; do
    case ${D}1 in
    -ord*)
        EXPORT_BY_ORDINALS=1;
        ;;
    -o*)
	shift
        outFile=${D}1
	;;
    -i*)
        shift
        outimpFile=${D}1
        ;;
    -name-mangler-script)
        shift
        renameScript=${D}1
        ;;
    -d*)
        shift
        description=${D}1
        ;;
    -f*)
        shift
        CFLAGS=${D}1
        ;;
    -c*)
        shift
        CC=${D}1
        ;;
    -h*)
        PrintHelp
        ;;
    -ex*)
        shift
        exclude_symbols=${D}{exclude_symbols}${D}1" "
        ;;
    -libf*)
        shift
        library_flags=${D}{library_flags}${D}1" "
        ;;
    -nocrt*)
        CFLAGS="-s"
        ;;
    -nolxl*)
        flag_USE_LXLITE=0
        ;;
    -* | /*)
        case ${D}1 in
        -L* | -l*)
            libsToLink=1
            ;;
        -Zomf)
            omfLinking=1
            ;;
        *)
            ;;
        esac
        EXTRA_CFLAGS=${D}{EXTRA_CFLAGS}" "${D}1
        ;;
    *.dll)
        EXTRA_CFLAGS="${D}{EXTRA_CFLAGS} \`basnam ${D}1 .dll\`"
        if @<:@ ${D}omfLinking -eq 1 @:>@; then
            EXTRA_CFLAGS="${D}{EXTRA_CFLAGS}.lib"
	else
            EXTRA_CFLAGS="${D}{EXTRA_CFLAGS}.a"
        fi
        ;;
    *)
        found=0;
        if @<:@ ${D}libsToLink -ne 0 @:>@; then
            EXTRA_CFLAGS=${D}{EXTRA_CFLAGS}" "${D}1
        else
            for file in ${D}1 ; do
                if @<:@ -f ${D}file @:>@; then
                    inputFiles="${D}{inputFiles} ${D}file"
                    found=1
                fi
            done
            if @<:@ ${D}found -eq 0 @:>@; then
                echo "ERROR: No file(s) found: "${D}1
                exit 8
            fi
        fi
      ;;
    esac
    shift
done # iterate cmdline words

#
if @<:@ -z "${D}inputFiles" @:>@; then
    echo "dllar: no input files"
    PrintHelp
fi

# Now extract all .o files from .a files
newInputFiles=""
for file in ${D}inputFiles ; do
    case ${D}file in
    *.a | *.lib)
        case ${D}file in
        *.a)
            suffix=".a"
            AR="ar"
            ;;
        *.lib)
            suffix=".lib"
            AR="emxomfar"
            EXTRA_CFLAGS="${D}EXTRA_CFLAGS -Zomf"
            ;;
        *)
            ;;
        esac
        dirname=\`basnam ${D}file ${D}suffix\`"_%"
        mkdir ${D}dirname
        if @<:@ ${D}? -ne 0 @:>@; then
            echo "Failed to create subdirectory ./${D}dirname"
            CleanUp
            exit 8;
        fi
        # Append '!' to indicate archive
        newInputFiles="${D}newInputFiles ${D}{dirname}!"
        doCommand "cd ${D}dirname; ${D}AR x ../${D}file"
        cd ${D}curDir
        found=0;
        for subfile in ${D}dirname/*.o* ; do
            if @<:@ -f ${D}subfile @:>@; then
                found=1
                if @<:@ -s ${D}subfile @:>@; then
	            # FIXME: This should be: is file size > 32 byte, _not_ > 0!
                    newInputFiles="${D}newInputFiles ${D}subfile"
                fi
            fi
        done
        if @<:@ ${D}found -eq 0 @:>@; then
            echo "WARNING: there are no files in archive \\'${D}file\\'"
        fi
        ;;
    *)
        newInputFiles="${D}{newInputFiles} ${D}file"
        ;;
    esac
done
inputFiles="${D}newInputFiles"

# Output filename(s).
do_backup=0;
if @<:@ -z ${D}outFile @:>@; then
    do_backup=1;
    set outFile ${D}inputFiles; outFile=${D}2
fi

# If it is an archive, remove the '!' and the '_%' suffixes
case ${D}outFile in
*_%!)
    outFile=\`basnam ${D}outFile _%!\`
    ;;
*)
    ;;
esac
case ${D}outFile in
*.dll)
    outFile=\`basnam ${D}outFile .dll\`
    ;;
*.DLL)
    outFile=\`basnam ${D}outFile .DLL\`
    ;;
*.o)
    outFile=\`basnam ${D}outFile .o\`
    ;;
*.obj)
    outFile=\`basnam ${D}outFile .obj\`
    ;;
*.a)
    outFile=\`basnam ${D}outFile .a\`
    ;;
*.lib)
    outFile=\`basnam ${D}outFile .lib\`
    ;;
*)
    ;;
esac
case ${D}outimpFile in
*.a)
    outimpFile=\`basnam ${D}outimpFile .a\`
    ;;
*.lib)
    outimpFile=\`basnam ${D}outimpFile .lib\`
    ;;
*)
    ;;
esac
if @<:@ -z ${D}outimpFile @:>@; then
    outimpFile=${D}outFile
fi
defFile="${D}{outFile}.def"
arcFile="${D}{outimpFile}.a"
arcFile2="${D}{outimpFile}.lib"

#create ${D}dllFile as something matching 8.3 restrictions,
if @<:@ -z ${D}renameScript @:>@ ; then
    dllFile="${D}outFile"
else
    dllFile=\`${D}renameScript ${D}outimpFile\`
fi

if @<:@ ${D}do_backup -ne 0 @:>@ ; then
    if @<:@ -f ${D}arcFile @:>@ ; then
        doCommand "mv ${D}arcFile ${D}{outFile}_s.a"
    fi
    if @<:@ -f ${D}arcFile2 @:>@ ; then
        doCommand "mv ${D}arcFile2 ${D}{outFile}_s.lib"
    fi
fi

# Extract public symbols from all the object files.
tmpdefFile=${D}{defFile}_%
rm -f ${D}tmpdefFile
for file in ${D}inputFiles ; do
    case ${D}file in
    *!)
        ;;
    *)
        doCommand "emxexp -u ${D}file >> ${D}tmpdefFile"
        ;;
    esac
done

# Create the def file.
rm -f ${D}defFile
echo "LIBRARY \`basnam ${D}dllFile\` ${D}library_flags" >> ${D}defFile
dllFile="${D}{dllFile}.dll"
if @<:@ ! -z ${D}description @:>@; then
    echo "DESCRIPTION  \\"${D}{description}\\"" >> ${D}defFile
fi
echo "EXPORTS" >> ${D}defFile

doCommand "cat ${D}tmpdefFile | sort.exe | uniq.exe > ${D}{tmpdefFile}%"
grep -v "^ *;" < ${D}{tmpdefFile}% | grep -v "^ *${D}" >${D}tmpdefFile

# Checks if the export is ok or not.
for word in ${D}exclude_symbols; do
    grep -v ${D}word < ${D}tmpdefFile >${D}{tmpdefFile}%
    mv ${D}{tmpdefFile}% ${D}tmpdefFile
done


if @<:@ ${D}EXPORT_BY_ORDINALS -ne 0 @:>@; then
    sed "=" < ${D}tmpdefFile | \\
    sed '
      N
      : loop
      s/^\\(@<:@0-9@:>@\\+\\)\\(@<:@^;@:>@*\\)\\(;.*\\)\\?/\\2 @\\1 NONAME/
      t loop
    ' > ${D}{tmpdefFile}%
    grep -v "^ *${D}" < ${D}{tmpdefFile}% > ${D}tmpdefFile
else
    rm -f ${D}{tmpdefFile}%
fi
cat ${D}tmpdefFile >> ${D}defFile
rm -f ${D}tmpdefFile

# Do linking, create implib, and apply lxlite.
gccCmdl="";
for file in ${D}inputFiles ; do
    case ${D}file in
    *!)
        ;;
    *)
        gccCmdl="${D}gccCmdl ${D}file"
        ;;
    esac
done
doCommand "${D}CC ${D}CFLAGS -Zdll -o ${D}dllFile ${D}defFile ${D}gccCmdl ${D}EXTRA_CFLAGS"
touch "${D}{outFile}.dll"

doCommand "emximp -o ${D}arcFile ${D}defFile"
if @<:@ ${D}flag_USE_LXLITE -ne 0 @:>@; then
    add_flags="";
    if @<:@ ${D}EXPORT_BY_ORDINALS -ne 0 @:>@; then
        add_flags="-ynd"
    fi
    doCommand "lxlite -cs -t: -mrn -mln ${D}add_flags ${D}dllFile"
fi
doCommand "emxomf -s -l ${D}arcFile"

# Successful exit.
CleanUp 1
exit 0
EOF
dnl ===================== dllar.sh ends here =====================
])

dnl
dnl  This file is part of Bakefile (http://www.bakefile.org)
dnl
dnl  Copyright (C) 2003-2007 Vaclav Slavik, David Elliott and others
dnl
dnl  Permission is hereby granted, free of charge, to any person obtaining a
dnl  copy of this software and associated documentation files (the "Software"),
dnl  to deal in the Software without restriction, including without limitation
dnl  the rights to use, copy, modify, merge, publish, distribute, sublicense,
dnl  and/or sell copies of the Software, and to permit persons to whom the
dnl  Software is furnished to do so, subject to the following conditions:
dnl
dnl  The above copyright notice and this permission notice shall be included in
dnl  all copies or substantial portions of the Software.
dnl
dnl  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
dnl  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
dnl  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
dnl  DEALINGS IN THE SOFTWARE.
dnl
dnl  $Id: bakefile-lang.m4 1337 2010-02-09 20:22:43Z vaclavslavik $
dnl
dnl  Compiler detection macros by David Elliott and Vadim Zeitlin
dnl


dnl ===========================================================================
dnl Macros to detect different C/C++ compilers
dnl ===========================================================================

dnl Based on autoconf _AC_LANG_COMPILER_GNU
dnl _AC_BAKEFILE_LANG_COMPILER(NAME, LANG, SYMBOL, IF-YES, IF-NO)
AC_DEFUN([_AC_BAKEFILE_LANG_COMPILER],
[
    AC_LANG_PUSH($2)
    AC_CACHE_CHECK(
        [whether we are using the $1 $2 compiler],
        [bakefile_cv_[]_AC_LANG_ABBREV[]_compiler_[]$3],
        [AC_TRY_COMPILE(
            [],
            [
             #ifndef $3
                choke me
             #endif
            ],
            [bakefile_cv_[]_AC_LANG_ABBREV[]_compiler_[]$3=yes],
            [bakefile_cv_[]_AC_LANG_ABBREV[]_compiler_[]$3=no]
         )
        ]
    )
    if test "x$bakefile_cv_[]_AC_LANG_ABBREV[]_compiler_[]$3" = "xyes"; then
        :; $4
    else
        :; $5
    fi
    AC_LANG_POP($2)
])

dnl More specific version of the above macro checking whether the compiler
dnl version is at least the given one (assumes that we do use this compiler)
dnl
dnl _AC_BAKEFILE_LANG_COMPILER_LATER_THAN(NAME, LANG, SYMBOL, VER, VERMSG, IF-YES, IF-NO)
AC_DEFUN([_AC_BAKEFILE_LANG_COMPILER_LATER_THAN],
[
    AC_LANG_PUSH($2)
    AC_CACHE_CHECK(
        [whether we are using $1 $2 compiler v$5 or later],
        [bakefile_cv_[]_AC_LANG_ABBREV[]_compiler_[]$3[]_lt_[]$4],
        [AC_TRY_COMPILE(
            [],
            [
             #ifndef $3 || $3 < $4
                choke me
             #endif
            ],
            [bakefile_cv_[]_AC_LANG_ABBREV[]_compiler_[]$3[]_lt_[]$4=yes],
            [bakefile_cv_[]_AC_LANG_ABBREV[]_compiler_[]$3[]_lt_[]$4=no]
         )
        ]
    )
    if test "x$bakefile_cv_[]_AC_LANG_ABBREV[]_compiler_[]$3[]_lt_[]$4" = "xyes"; then
        :; $6
    else
        :; $7
    fi
    AC_LANG_POP($2)
])

dnl CodeWarrior Metrowerks compiler defines __MWERKS__ for both C and C++
AC_DEFUN([AC_BAKEFILE_PROG_MWCC],
[
    _AC_BAKEFILE_LANG_COMPILER(Metrowerks, C, __MWERKS__, MWCC=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_MWCXX],
[
    _AC_BAKEFILE_LANG_COMPILER(Metrowerks, C++, __MWERKS__, MWCXX=yes)
])

dnl IBM xlC compiler defines __xlC__ for both C and C++
AC_DEFUN([AC_BAKEFILE_PROG_XLCC],
[
    _AC_BAKEFILE_LANG_COMPILER([IBM xlC], C, __xlC__, XLCC=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_XLCXX],
[
    _AC_BAKEFILE_LANG_COMPILER([IBM xlC], C++, __xlC__, XLCXX=yes)
])

dnl recent versions of SGI mipsPro compiler define _SGI_COMPILER_VERSION
dnl
dnl NB: old versions define _COMPILER_VERSION but this could probably be
dnl     defined by other compilers too so don't test for it to be safe
AC_DEFUN([AC_BAKEFILE_PROG_SGICC],
[
    _AC_BAKEFILE_LANG_COMPILER(SGI, C, _SGI_COMPILER_VERSION, SGICC=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_SGICXX],
[
    _AC_BAKEFILE_LANG_COMPILER(SGI, C++, _SGI_COMPILER_VERSION, SGICXX=yes)
])

dnl Sun compiler defines __SUNPRO_C/__SUNPRO_CC
AC_DEFUN([AC_BAKEFILE_PROG_SUNCC],
[
    _AC_BAKEFILE_LANG_COMPILER(Sun, C, __SUNPRO_C, SUNCC=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_SUNCXX],
[
    _AC_BAKEFILE_LANG_COMPILER(Sun, C++, __SUNPRO_CC, SUNCXX=yes)
])

dnl Intel icc compiler defines __INTEL_COMPILER for both C and C++
AC_DEFUN([AC_BAKEFILE_PROG_INTELCC],
[
    _AC_BAKEFILE_LANG_COMPILER(Intel, C, __INTEL_COMPILER, INTELCC=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_INTELCXX],
[
    _AC_BAKEFILE_LANG_COMPILER(Intel, C++, __INTEL_COMPILER, INTELCXX=yes)
])

dnl Intel compiler command line options changed in incompatible ways sometimes
dnl before v8 (-KPIC was replaced with gcc-compatible -fPIC) and again in v10
dnl (-create-pch deprecated in favour of -pch-create) so we need to test for
dnl its exact version too
AC_DEFUN([AC_BAKEFILE_PROG_INTELCC_8],
[
    _AC_BAKEFILE_LANG_COMPILER_LATER_THAN(Intel, C, __INTEL_COMPILER, 800, 8, INTELCC8=yes)
])
AC_DEFUN([AC_BAKEFILE_PROG_INTELCXX_8],
[
    _AC_BAKEFILE_LANG_COMPILER_LATER_THAN(Intel, C++, __INTEL_COMPILER, 800, 8, INTELCXX8=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_INTELCC_10],
[
    _AC_BAKEFILE_LANG_COMPILER_LATER_THAN(Intel, C, __INTEL_COMPILER, 1000, 10, INTELCC10=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_INTELCXX_10],
[
    _AC_BAKEFILE_LANG_COMPILER_LATER_THAN(Intel, C++, __INTEL_COMPILER, 1000, 10, INTELCXX10=yes)
])

dnl HP-UX aCC: see http://docs.hp.com/en/6162/preprocess.htm#macropredef
AC_DEFUN([AC_BAKEFILE_PROG_HPCC],
[
    _AC_BAKEFILE_LANG_COMPILER(HP, C, __HP_cc, HPCC=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_HPCXX],
[
    _AC_BAKEFILE_LANG_COMPILER(HP, C++, __HP_aCC, HPCXX=yes)
])

dnl Tru64 cc and cxx
AC_DEFUN([AC_BAKEFILE_PROG_COMPAQCC],
[
    _AC_BAKEFILE_LANG_COMPILER(Compaq, C, __DECC, COMPAQCC=yes)
])

AC_DEFUN([AC_BAKEFILE_PROG_COMPAQCXX],
[
    _AC_BAKEFILE_LANG_COMPILER(Compaq, C++, __DECCXX, COMPAQCXX=yes)
])

dnl ===========================================================================
dnl macros to detect specialty compiler options
dnl ===========================================================================

dnl Figure out if we need to pass -ext o to compiler (MetroWerks)
AC_DEFUN([AC_BAKEFILE_METROWERKS_EXTO],
[AC_CACHE_CHECK([if the _AC_LANG compiler requires -ext o], bakefile_cv_[]_AC_LANG_ABBREV[]_exto,
dnl First create an empty conf test
[AC_LANG_CONFTEST([AC_LANG_PROGRAM()])
dnl Now remove .o and .c.o or .cc.o
rm -f conftest.$ac_objext conftest.$ac_ext.o
dnl Now compile the test
AS_IF([AC_TRY_EVAL(ac_compile)],
dnl If the test succeeded look for conftest.c.o or conftest.cc.o
[for ac_file in `(ls conftest.* 2>/dev/null)`; do
    case $ac_file in
        conftest.$ac_ext.o)
            bakefile_cv_[]_AC_LANG_ABBREV[]_exto="-ext o"
            ;;
        *)
            ;;
    esac
done],
[AC_MSG_FAILURE([cannot figure out if compiler needs -ext o: cannot compile])
]) dnl AS_IF

rm -f conftest.$ac_ext.o conftest.$ac_objext conftest.$ac_ext
]) dnl AC_CACHE_CHECK

if test "x$bakefile_cv_[]_AC_LANG_ABBREV[]_exto" '!=' "x"; then
    if test "[]_AC_LANG_ABBREV[]" = "c"; then
        CFLAGS="$bakefile_cv_[]_AC_LANG_ABBREV[]_exto $CFLAGS"
    fi
    if test "[]_AC_LANG_ABBREV[]" = "cxx"; then
        CXXFLAGS="$bakefile_cv_[]_AC_LANG_ABBREV[]_exto $CXXFLAGS"
    fi
fi
]) dnl AC_DEFUN


dnl ===========================================================================
dnl Macros to do all of the compiler detections as one macro
dnl ===========================================================================

dnl check for different proprietary compilers depending on target platform
dnl _AC_BAKEFILE_PROG_COMPILER(LANG)
AC_DEFUN([_AC_BAKEFILE_PROG_COMPILER],
[
    AC_REQUIRE([AC_PROG_$1])

    dnl Intel compiler can be used under several different OS and even
    dnl different architectures (x86, amd64 and Itanium) so it's easier to just
    dnl always test for it
    AC_BAKEFILE_PROG_INTEL$1

    dnl If we use Intel compiler we also need to know its version
    if test "$INTEL$1" = "yes"; then
        AC_BAKEFILE_PROG_INTEL$1_8
        AC_BAKEFILE_PROG_INTEL$1_10
    fi

    dnl if we're using gcc, we can't be using any of incompatible compilers
    if test "x$G$1" != "xyes"; then
        if test "x$1" = "xC"; then
            AC_BAKEFILE_METROWERKS_EXTO
            if test "x$bakefile_cv_c_exto" '!=' "x"; then
                unset ac_cv_prog_cc_g
                _AC_PROG_CC_G
            fi
        fi

        dnl most of these compilers are only used under well-defined OS so
        dnl don't waste time checking for them on other ones
        case `uname -s` in
            AIX*)
                AC_BAKEFILE_PROG_XL$1
                ;;

            Darwin)
                AC_BAKEFILE_PROG_MW$1
                if test "$MW$1" != "yes"; then
                    AC_BAKEFILE_PROG_XL$1
                fi
                ;;

            IRIX*)
                AC_BAKEFILE_PROG_SGI$1
                ;;

            Linux*)
                dnl Sun CC is now available under Linux too, test for it unless
                dnl we already found that we were using a different compiler
                if test "$INTEL$1" != "yes"; then
                    AC_BAKEFILE_PROG_SUN$1
                fi
                ;;

            HP-UX*)
                AC_BAKEFILE_PROG_HP$1
                ;;

            OSF1)
                AC_BAKEFILE_PROG_COMPAQ$1
                ;;

            SunOS)
                AC_BAKEFILE_PROG_SUN$1
                ;;
        esac
    fi
])

AC_DEFUN([AC_BAKEFILE_PROG_CC],
[
    _AC_BAKEFILE_PROG_COMPILER(CC)
])

AC_DEFUN([AC_BAKEFILE_PROG_CXX],
[
    _AC_BAKEFILE_PROG_COMPILER(CXX)
])


dnl
dnl  This file is part of Bakefile (http://www.bakefile.org)
dnl
dnl  Copyright (C) 2003-2007 Vaclav Slavik and others
dnl
dnl  Permission is hereby granted, free of charge, to any person obtaining a
dnl  copy of this software and associated documentation files (the "Software"),
dnl  to deal in the Software without restriction, including without limitation
dnl  the rights to use, copy, modify, merge, publish, distribute, sublicense,
dnl  and/or sell copies of the Software, and to permit persons to whom the
dnl  Software is furnished to do so, subject to the following conditions:
dnl
dnl  The above copyright notice and this permission notice shall be included in
dnl  all copies or substantial portions of the Software.
dnl
dnl  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
dnl  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
dnl  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
dnl  DEALINGS IN THE SOFTWARE.
dnl
dnl  $Id: bakefile.m4 1346 2011-02-01 14:03:00Z vaclavslavik $
dnl
dnl  Support macros for makefiles generated by BAKEFILE.
dnl


dnl ---------------------------------------------------------------------------
dnl Lots of compiler & linker detection code contained here was taken from
dnl wxWidgets configure.in script (see http://www.wxwidgets.org)
dnl ---------------------------------------------------------------------------



dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_GNUMAKE
dnl
dnl Detects GNU make
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_GNUMAKE],
[
    dnl does make support "-include" (only GNU make does AFAIK)?
    AC_CACHE_CHECK([if make is GNU make], bakefile_cv_prog_makeisgnu,
    [
        if ( ${SHELL-sh} -c "${MAKE-make} --version" 2> /dev/null |
                egrep -s GNU > /dev/null); then
            bakefile_cv_prog_makeisgnu="yes"
        else
            bakefile_cv_prog_makeisgnu="no"
        fi
    ])

    if test "x$bakefile_cv_prog_makeisgnu" = "xyes"; then
        IF_GNU_MAKE=""
    else
        IF_GNU_MAKE="#"
    fi
    AC_SUBST(IF_GNU_MAKE)
])

dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_PLATFORM
dnl
dnl Detects platform and sets PLATFORM_XXX variables accordingly
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_PLATFORM],
[
    PLATFORM_UNIX=0
    PLATFORM_WIN32=0
    PLATFORM_MSDOS=0
    PLATFORM_MAC=0
    PLATFORM_MACOS=0
    PLATFORM_MACOSX=0
    PLATFORM_OS2=0
    PLATFORM_BEOS=0

    if test "x$BAKEFILE_FORCE_PLATFORM" = "x"; then
        case "${BAKEFILE_HOST}" in
            *-*-mingw32* )
                PLATFORM_WIN32=1
            ;;
            *-pc-msdosdjgpp )
                PLATFORM_MSDOS=1
            ;;
            *-pc-os2_emx | *-pc-os2-emx )
                PLATFORM_OS2=1
            ;;
            *-*-darwin* )
                PLATFORM_MAC=1
                PLATFORM_MACOSX=1
            ;;
            *-*-beos* )
                PLATFORM_BEOS=1
            ;;
            powerpc-apple-macos* )
                PLATFORM_MAC=1
                PLATFORM_MACOS=1
            ;;
            * )
                PLATFORM_UNIX=1
            ;;
        esac
    else
        case "$BAKEFILE_FORCE_PLATFORM" in
            win32 )
                PLATFORM_WIN32=1
            ;;
            msdos )
                PLATFORM_MSDOS=1
            ;;
            os2 )
                PLATFORM_OS2=1
            ;;
            darwin )
                PLATFORM_MAC=1
                PLATFORM_MACOSX=1
            ;;
            unix )
                PLATFORM_UNIX=1
            ;;
            beos )
                PLATFORM_BEOS=1
            ;;
            * )
                AC_MSG_ERROR([Unknown platform: $BAKEFILE_FORCE_PLATFORM])
            ;;
        esac
    fi

    AC_SUBST(PLATFORM_UNIX)
    AC_SUBST(PLATFORM_WIN32)
    AC_SUBST(PLATFORM_MSDOS)
    AC_SUBST(PLATFORM_MAC)
    AC_SUBST(PLATFORM_MACOS)
    AC_SUBST(PLATFORM_MACOSX)
    AC_SUBST(PLATFORM_OS2)
    AC_SUBST(PLATFORM_BEOS)
])


dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_PLATFORM_SPECIFICS
dnl
dnl Sets misc platform-specific settings
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_PLATFORM_SPECIFICS],
[
    AC_ARG_ENABLE([omf], AS_HELP_STRING([--enable-omf],
                                        [use OMF object format (OS/2)]),
                  [bk_os2_use_omf="$enableval"])

    case "${BAKEFILE_HOST}" in
      *-*-darwin* )
        dnl For Unix to MacOS X porting instructions, see:
        dnl http://fink.sourceforge.net/doc/porting/porting.html
        if test "x$GCC" = "xyes"; then
            CFLAGS="$CFLAGS -fno-common"
            CXXFLAGS="$CXXFLAGS -fno-common"
        fi
        if test "x$XLCC" = "xyes"; then
            CFLAGS="$CFLAGS -qnocommon"
            CXXFLAGS="$CXXFLAGS -qnocommon"
        fi
        ;;

      *-pc-os2_emx | *-pc-os2-emx )
        if test "x$bk_os2_use_omf" = "xyes" ; then
            AR=emxomfar
            RANLIB=:
            LDFLAGS="-Zomf $LDFLAGS"
            CFLAGS="-Zomf $CFLAGS"
            CXXFLAGS="-Zomf $CXXFLAGS"
            OS2_LIBEXT="lib"
        else
            OS2_LIBEXT="a"
        fi
        ;;

      i*86-*-beos* )
        LDFLAGS="-L/boot/develop/lib/x86 $LDFLAGS"
        ;;
    esac
])

dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_SUFFIXES
dnl
dnl Detects shared various suffixes for shared libraries, libraries, programs,
dnl plugins etc.
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_SUFFIXES],
[
    SO_SUFFIX="so"
    SO_SUFFIX_MODULE="so"
    EXEEXT=""
    LIBPREFIX="lib"
    LIBEXT=".a"
    DLLPREFIX="lib"
    DLLPREFIX_MODULE=""
    DLLIMP_SUFFIX=""
    dlldir="$libdir"

    case "${BAKEFILE_HOST}" in
        dnl PA-RISC HP systems used .sl but IA64 use ELF-64 and so use the
        dnl standard .so extension
        ia64-hp-hpux* )
        ;;
        *-hp-hpux* )
            SO_SUFFIX="sl"
            SO_SUFFIX_MODULE="sl"
        ;;
        *-*-aix* )
            dnl quoting from
            dnl http://www-1.ibm.com/servers/esdd/articles/gnu.html:
            dnl     Both archive libraries and shared libraries on AIX have an
            dnl     .a extension. This will explain why you can't link with an
            dnl     .so and why it works with the name changed to .a.
            SO_SUFFIX="a"
            SO_SUFFIX_MODULE="a"
        ;;
        *-*-cygwin* )
            SO_SUFFIX="dll"
            SO_SUFFIX_MODULE="dll"
            DLLIMP_SUFFIX="dll.a"
            EXEEXT=".exe"
            DLLPREFIX="cyg"
            dlldir="$bindir"
        ;;
        *-*-mingw32* )
            SO_SUFFIX="dll"
            SO_SUFFIX_MODULE="dll"
            DLLIMP_SUFFIX="dll.a"
            EXEEXT=".exe"
            DLLPREFIX=""
            dlldir="$bindir"
        ;;
        *-pc-msdosdjgpp )
            EXEEXT=".exe"
            DLLPREFIX=""
            dlldir="$bindir"
        ;;
        *-pc-os2_emx | *-pc-os2-emx )
            SO_SUFFIX="dll"
            SO_SUFFIX_MODULE="dll"
            DLLIMP_SUFFIX=$OS2_LIBEXT
            EXEEXT=".exe"
            DLLPREFIX=""
            LIBPREFIX=""
            LIBEXT=".$OS2_LIBEXT"
            dlldir="$bindir"
        ;;
        *-*-darwin* )
            SO_SUFFIX="dylib"
            SO_SUFFIX_MODULE="bundle"
        ;;
    esac

    if test "x$DLLIMP_SUFFIX" = "x" ; then
        DLLIMP_SUFFIX="$SO_SUFFIX"
    fi

    AC_SUBST(SO_SUFFIX)
    AC_SUBST(SO_SUFFIX_MODULE)
    AC_SUBST(DLLIMP_SUFFIX)
    AC_SUBST(EXEEXT)
    AC_SUBST(LIBPREFIX)
    AC_SUBST(LIBEXT)
    AC_SUBST(DLLPREFIX)
    AC_SUBST(DLLPREFIX_MODULE)
    AC_SUBST(dlldir)
])


dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_SHARED_LD
dnl
dnl Detects command for making shared libraries, substitutes SHARED_LD_CC
dnl and SHARED_LD_CXX.
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_SHARED_LD],
[
    dnl the extra compiler flags needed for compilation of shared library
    PIC_FLAG=""
    if test "x$GCC" = "xyes"; then
        dnl the switch for gcc is the same under all platforms
        PIC_FLAG="-fPIC"
    fi

    dnl Defaults for GCC and ELF .so shared libs:
    SHARED_LD_CC="\$(CC) -shared ${PIC_FLAG} -o"
    SHARED_LD_CXX="\$(CXX) -shared ${PIC_FLAG} -o"
    WINDOWS_IMPLIB=0

    case "${BAKEFILE_HOST}" in
      *-hp-hpux* )
        dnl default settings are good for gcc but not for the native HP-UX
        if test "x$GCC" != "xyes"; then
            dnl no idea why it wants it, but it does
            LDFLAGS="$LDFLAGS -L/usr/lib"

            SHARED_LD_CC="${CC} -b -o"
            SHARED_LD_CXX="${CXX} -b -o"
            PIC_FLAG="+Z"
        fi
      ;;

      *-*-linux* )
        dnl newer icc versions use -fPIC just as gcc does and, in fact, the
        dnl newest (v10+) ones don't even understand -KPIC any longer
        if test "$INTELCC" = "yes" -a "$INTELCC8" != "yes"; then
            PIC_FLAG="-KPIC"
        elif test "x$SUNCXX" = "xyes"; then
            SHARED_LD_CC="${CC} -G -o"
            SHARED_LD_CXX="${CXX} -G -o"
            PIC_FLAG="-KPIC"
        fi
      ;;

      *-*-solaris2* )
        if test "x$SUNCXX" = xyes ; then
            SHARED_LD_CC="${CC} -G -o"
            SHARED_LD_CXX="${CXX} -G -o"
            PIC_FLAG="-KPIC"
        fi
      ;;

      *-*-darwin* )
        AC_BAKEFILE_CREATE_FILE_SHARED_LD_SH
        chmod +x shared-ld-sh

        SHARED_LD_MODULE_CC="`pwd`/shared-ld-sh -bundle -headerpad_max_install_names -o"
        SHARED_LD_MODULE_CXX="CXX=\"\$(CXX)\" $SHARED_LD_MODULE_CC"

        dnl Most apps benefit from being fully binded (its faster and static
        dnl variables initialized at startup work).
        dnl This can be done either with the exe linker flag -Wl,-bind_at_load
        dnl or with a double stage link in order to create a single module
        dnl "-init _wxWindowsDylibInit" not useful with lazy linking solved

        dnl If using newer dev tools then there is a -single_module flag that
        dnl we can use to do this for dylibs, otherwise we'll need to use a helper
        dnl script.  Check the version of gcc to see which way we can go:
        AC_CACHE_CHECK([for gcc 3.1 or later], bakefile_cv_gcc31, [
           AC_TRY_COMPILE([],
               [
                   #if (__GNUC__ < 3) || \
                       ((__GNUC__ == 3) && (__GNUC_MINOR__ < 1))
                       This is old gcc
                   #endif
               ],
               [
                   bakefile_cv_gcc31=yes
               ],
               [
                   bakefile_cv_gcc31=no
               ]
           )
        ])
        if test "$bakefile_cv_gcc31" = "no"; then
            dnl Use the shared-ld-sh helper script
            SHARED_LD_CC="`pwd`/shared-ld-sh -dynamiclib -headerpad_max_install_names -o"
            SHARED_LD_CXX="$SHARED_LD_CC"
        else
            dnl Use the -single_module flag and let the linker do it for us
            SHARED_LD_CC="\${CC} -dynamiclib -single_module -headerpad_max_install_names -o"
            SHARED_LD_CXX="\${CXX} -dynamiclib -single_module -headerpad_max_install_names -o"
        fi

        if test "x$GCC" == "xyes"; then
            PIC_FLAG="-dynamic -fPIC"
        fi
        if test "x$XLCC" = "xyes"; then
            PIC_FLAG="-dynamic -DPIC"
        fi
      ;;

      *-*-aix* )
        if test "x$GCC" = "xyes"; then
            dnl at least gcc 2.95 warns that -fPIC is ignored when
            dnl compiling each and every file under AIX which is annoying,
            dnl so don't use it there (it's useless as AIX runs on
            dnl position-independent architectures only anyhow)
            PIC_FLAG=""

            dnl -bexpfull is needed by AIX linker to export all symbols (by
            dnl default it doesn't export any and even with -bexpall it
            dnl doesn't export all C++ support symbols, e.g. vtable
            dnl pointers) but it's only available starting from 5.1 (with
            dnl maintenance pack 2, whatever this is), see
            dnl http://www-128.ibm.com/developerworks/eserver/articles/gnu.html
            case "${BAKEFILE_HOST}" in
                *-*-aix5* )
                    LD_EXPFULL="-Wl,-bexpfull"
                    ;;
            esac

            SHARED_LD_CC="\$(CC) -shared $LD_EXPFULL -o"
            SHARED_LD_CXX="\$(CXX) -shared $LD_EXPFULL -o"
        else
            dnl FIXME: makeC++SharedLib is obsolete, what should we do for
            dnl        recent AIX versions?
            AC_CHECK_PROG(AIX_CXX_LD, makeC++SharedLib,
                          makeC++SharedLib, /usr/lpp/xlC/bin/makeC++SharedLib)
            SHARED_LD_CC="$AIX_CC_LD -p 0 -o"
            SHARED_LD_CXX="$AIX_CXX_LD -p 0 -o"
        fi
      ;;

      *-*-beos* )
        dnl can't use gcc under BeOS for shared library creation because it
        dnl complains about missing 'main'
        SHARED_LD_CC="${LD} -nostart -o"
        SHARED_LD_CXX="${LD} -nostart -o"
      ;;

      *-*-irix* )
        dnl default settings are ok for gcc
        if test "x$GCC" != "xyes"; then
            PIC_FLAG="-KPIC"
        fi
      ;;

      *-*-cygwin* | *-*-mingw32* )
        PIC_FLAG=""
        SHARED_LD_CC="\$(CC) -shared -o"
        SHARED_LD_CXX="\$(CXX) -shared -o"
        WINDOWS_IMPLIB=1
      ;;

      *-pc-os2_emx | *-pc-os2-emx )
        SHARED_LD_CC="`pwd`/dllar.sh -libf INITINSTANCE -libf TERMINSTANCE -o"
        SHARED_LD_CXX="`pwd`/dllar.sh -libf INITINSTANCE -libf TERMINSTANCE -o"
        PIC_FLAG=""
        AC_BAKEFILE_CREATE_FILE_DLLAR_SH
        chmod +x dllar.sh
      ;;

      powerpc-apple-macos* | \
      *-*-freebsd* | *-*-openbsd* | *-*-netbsd* | *-*-k*bsd*-gnu | \
      *-*-mirbsd* | \
      *-*-sunos4* | \
      *-*-osf* | \
      *-*-dgux5* | \
      *-*-sysv5* | \
      *-pc-msdosdjgpp )
        dnl defaults are ok
      ;;

      *)
        AC_MSG_ERROR(unknown system type $BAKEFILE_HOST.)
    esac

    if test "x$PIC_FLAG" != "x" ; then
        PIC_FLAG="$PIC_FLAG -DPIC"
    fi

    if test "x$SHARED_LD_MODULE_CC" = "x" ; then
        SHARED_LD_MODULE_CC="$SHARED_LD_CC"
    fi
    if test "x$SHARED_LD_MODULE_CXX" = "x" ; then
        SHARED_LD_MODULE_CXX="$SHARED_LD_CXX"
    fi

    AC_SUBST(SHARED_LD_CC)
    AC_SUBST(SHARED_LD_CXX)
    AC_SUBST(SHARED_LD_MODULE_CC)
    AC_SUBST(SHARED_LD_MODULE_CXX)
    AC_SUBST(PIC_FLAG)
    AC_SUBST(WINDOWS_IMPLIB)
])


dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_SHARED_VERSIONS
dnl
dnl Detects linker options for attaching versions (sonames) to shared  libs.
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_SHARED_VERSIONS],
[
    USE_SOVERSION=0
    USE_SOVERLINUX=0
    USE_SOVERSOLARIS=0
    USE_SOVERCYGWIN=0
    USE_SOTWOSYMLINKS=0
    USE_MACVERSION=0
    SONAME_FLAG=

    case "${BAKEFILE_HOST}" in
      *-*-linux* | *-*-freebsd* | *-*-openbsd* | *-*-netbsd* | \
      *-*-k*bsd*-gnu | *-*-mirbsd* )
        if test "x$SUNCXX" = "xyes"; then
            SONAME_FLAG="-h "
        else
            SONAME_FLAG="-Wl,-soname,"
        fi
        USE_SOVERSION=1
        USE_SOVERLINUX=1
        USE_SOTWOSYMLINKS=1
      ;;

      *-*-solaris2* )
        SONAME_FLAG="-h "
        USE_SOVERSION=1
        USE_SOVERSOLARIS=1
      ;;

      *-*-darwin* )
        USE_MACVERSION=1
        USE_SOVERSION=1
        USE_SOTWOSYMLINKS=1
      ;;

      *-*-cygwin* )
        USE_SOVERSION=1
        USE_SOVERCYGWIN=1
      ;;
    esac

    AC_SUBST(USE_SOVERSION)
    AC_SUBST(USE_SOVERLINUX)
    AC_SUBST(USE_SOVERSOLARIS)
    AC_SUBST(USE_SOVERCYGWIN)
    AC_SUBST(USE_MACVERSION)
    AC_SUBST(USE_SOTWOSYMLINKS)
    AC_SUBST(SONAME_FLAG)
])


dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_DEPS
dnl
dnl Detects available C/C++ dependency tracking options
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_DEPS],
[
    AC_ARG_ENABLE([dependency-tracking],
                  AS_HELP_STRING([--disable-dependency-tracking],
                                 [don't use dependency tracking even if the compiler can]),
                  [bk_use_trackdeps="$enableval"])

    AC_MSG_CHECKING([for dependency tracking method])

    BK_DEPS=""
    if test "x$bk_use_trackdeps" = "xno" ; then
        DEPS_TRACKING=0
        AC_MSG_RESULT([disabled])
    else
        DEPS_TRACKING=1

        if test "x$GCC" = "xyes"; then
            DEPSMODE=gcc
            case "${BAKEFILE_HOST}" in
                *-*-darwin* )
                    dnl -cpp-precomp (the default) conflicts with -MMD option
                    dnl used by bk-deps (see also http://developer.apple.com/documentation/Darwin/Conceptual/PortingUnix/compiling/chapter_4_section_3.html)
                    DEPSFLAG="-no-cpp-precomp -MMD"
                ;;
                * )
                    DEPSFLAG="-MMD"
                ;;
            esac
            AC_MSG_RESULT([gcc])
        elif test "x$MWCC" = "xyes"; then
            DEPSMODE=mwcc
            DEPSFLAG="-MM"
            AC_MSG_RESULT([mwcc])
        elif test "x$SUNCC" = "xyes"; then
            DEPSMODE=unixcc
            DEPSFLAG="-xM1"
            AC_MSG_RESULT([Sun cc])
        elif test "x$SGICC" = "xyes"; then
            DEPSMODE=unixcc
            DEPSFLAG="-M"
            AC_MSG_RESULT([SGI cc])
        elif test "x$HPCC" = "xyes"; then
            DEPSMODE=unixcc
            DEPSFLAG="+make"
            AC_MSG_RESULT([HP cc])
        elif test "x$COMPAQCC" = "xyes"; then
            DEPSMODE=gcc
            DEPSFLAG="-MD"
            AC_MSG_RESULT([Compaq cc])
        else
            DEPS_TRACKING=0
            AC_MSG_RESULT([none])
        fi

        if test $DEPS_TRACKING = 1 ; then
            AC_BAKEFILE_CREATE_FILE_BK_DEPS
            chmod +x bk-deps
            dnl FIXME: make this $(top_builddir)/bk-deps once autoconf-2.60
            dnl        is required (and so top_builddir is never empty):
            BK_DEPS="`pwd`/bk-deps"
        fi
    fi

    AC_SUBST(DEPS_TRACKING)
    AC_SUBST(BK_DEPS)
])

dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_CHECK_BASIC_STUFF
dnl
dnl Checks for presence of basic programs, such as C and C++ compiler, "ranlib"
dnl or "install"
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_CHECK_BASIC_STUFF],
[
    AC_PROG_RANLIB
    AC_PROG_INSTALL
    AC_PROG_LN_S

    AC_PROG_MAKE_SET
    AC_SUBST(MAKE_SET)

    if test "x$SUNCXX" = "xyes"; then
        dnl Sun C++ compiler requires special way of creating static libs;
        dnl see here for more details:
        dnl https://sourceforge.net/tracker/?func=detail&atid=109863&aid=1229751&group_id=9863
        AR=$CXX
        AROPTIONS="-xar -o"
        AC_SUBST(AR)
    elif test "x$SGICC" = "xyes"; then
        dnl Almost the same as above for SGI mipsPro compiler
        AR=$CXX
        AROPTIONS="-ar -o"
        AC_SUBST(AR)
    else
        AC_CHECK_TOOL(AR, ar, ar)
        AROPTIONS=rcu
    fi
    AC_SUBST(AROPTIONS)

    AC_CHECK_TOOL(STRIP, strip, :)
    AC_CHECK_TOOL(NM, nm, :)

    dnl This check is necessary because "install -d" doesn't exist on
    dnl all platforms (e.g. HP/UX), see http://www.bakefile.org/ticket/80
    AC_MSG_CHECKING([for command to install directories])
    INSTALL_TEST_DIR=acbftest$$
    $INSTALL -d $INSTALL_TEST_DIR > /dev/null 2>&1
    if test $? = 0 -a -d $INSTALL_TEST_DIR; then
        rmdir $INSTALL_TEST_DIR
        dnl we must refer to makefile's $(INSTALL) variable and not
        dnl current value of shell variable, hence the single quoting:
        INSTALL_DIR='$(INSTALL) -d'
        AC_MSG_RESULT([$INSTALL -d])
    else
        INSTALL_DIR="mkdir -p"
        AC_MSG_RESULT([mkdir -p])
    fi
    AC_SUBST(INSTALL_DIR)

    LDFLAGS_GUI=
    case ${BAKEFILE_HOST} in
        *-*-cygwin* | *-*-mingw32* )
        LDFLAGS_GUI="-mwindows"
    esac
    AC_SUBST(LDFLAGS_GUI)
])


dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_RES_COMPILERS
dnl
dnl Checks for presence of resource compilers for win32 or mac
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_RES_COMPILERS],
[
    case ${BAKEFILE_HOST} in
        *-*-cygwin* | *-*-mingw32* )
            dnl Check for win32 resources compiler:
            AC_CHECK_TOOL(WINDRES, windres)
         ;;

      *-*-darwin* | powerpc-apple-macos* )
            AC_CHECK_PROG(REZ, Rez, Rez, /Developer/Tools/Rez)
            AC_CHECK_PROG(SETFILE, SetFile, SetFile, /Developer/Tools/SetFile)
        ;;
    esac

    AC_SUBST(WINDRES)
    AC_SUBST(REZ)
    AC_SUBST(SETFILE)
])

dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE_PRECOMP_HEADERS
dnl
dnl Check for precompiled headers support (GCC >= 3.4)
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_PRECOMP_HEADERS],
[

    AC_ARG_ENABLE([precomp-headers],
                  AS_HELP_STRING([--disable-precomp-headers],
                                 [don't use precompiled headers even if compiler can]),
                  [bk_use_pch="$enableval"])

    GCC_PCH=0
    ICC_PCH=0
    USE_PCH=0
    BK_MAKE_PCH=""

    case ${BAKEFILE_HOST} in
        *-*-cygwin* )
            dnl PCH support is broken in cygwin gcc because of unportable
            dnl assumptions about mmap() in gcc code which make PCH generation
            dnl fail erratically; disable PCH completely until this is fixed
            bk_use_pch="no"
            ;;
    esac

    if test "x$bk_use_pch" = "x" -o "x$bk_use_pch" = "xyes" ; then
        if test "x$GCC" = "xyes"; then
            dnl test if we have gcc-3.4:
            AC_MSG_CHECKING([if the compiler supports precompiled headers])
            AC_TRY_COMPILE([],
                [
                    #if !defined(__GNUC__) || !defined(__GNUC_MINOR__)
                        There is no PCH support
                    #endif
                    #if (__GNUC__ < 3)
                        There is no PCH support
                    #endif
                    #if (__GNUC__ == 3) && \
                       ((!defined(__APPLE_CC__) && (__GNUC_MINOR__ < 4)) || \
                       ( defined(__APPLE_CC__) && (__GNUC_MINOR__ < 3))) || \
                       ( defined(__INTEL_COMPILER) )
                        There is no PCH support
                    #endif
                ],
                [
                    AC_MSG_RESULT([yes])
                    GCC_PCH=1
                ],
                [
                    if test "$INTELCXX8" = "yes"; then
                        AC_MSG_RESULT([yes])
                        ICC_PCH=1
                        if test "$INTELCXX10" = "yes"; then
                            ICC_PCH_CREATE_SWITCH="-pch-create"
                            ICC_PCH_USE_SWITCH="-pch-use"
                        else
                            ICC_PCH_CREATE_SWITCH="-create-pch"
                            ICC_PCH_USE_SWITCH="-use-pch"
                        fi
                    else
                        AC_MSG_RESULT([no])
                    fi
                ])
            if test $GCC_PCH = 1 -o $ICC_PCH = 1 ; then
                USE_PCH=1
                AC_BAKEFILE_CREATE_FILE_BK_MAKE_PCH
                chmod +x bk-make-pch
                dnl FIXME: make this $(top_builddir)/bk-make-pch once
                dnl        autoconf-2.60 is required (and so top_builddir is
                dnl        never empty):
                BK_MAKE_PCH="`pwd`/bk-make-pch"
            fi
        fi
    fi

    AC_SUBST(GCC_PCH)
    AC_SUBST(ICC_PCH)
    AC_SUBST(ICC_PCH_CREATE_SWITCH)
    AC_SUBST(ICC_PCH_USE_SWITCH)
    AC_SUBST(BK_MAKE_PCH)
])



dnl ---------------------------------------------------------------------------
dnl AC_BAKEFILE([autoconf_inc.m4 inclusion])
dnl
dnl To be used in configure.in of any project using Bakefile-generated mks
dnl
dnl Behaviour can be modified by setting following variables:
dnl    BAKEFILE_CHECK_BASICS    set to "no" if you don't want bakefile to
dnl                             to perform check for basic tools like ranlib
dnl    BAKEFILE_HOST            set this to override host detection, defaults
dnl                             to ${host}
dnl    BAKEFILE_FORCE_PLATFORM  set to override platform detection
dnl
dnl Example usage:
dnl
dnl   AC_BAKEFILE([FOO(autoconf_inc.m4)])
dnl
dnl (replace FOO with m4_include above, aclocal would die otherwise)
dnl (yes, it's ugly, but thanks to a bug in aclocal, it's the only thing
dnl we can do...)
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE],
[
    AC_PREREQ([2.58])

    dnl We need to always run C/C++ compiler tests, but it's also possible
    dnl for the user to call these macros manually, hence this instead of
    dnl simply calling these macros. See http://www.bakefile.org/ticket/64
    AC_REQUIRE([AC_BAKEFILE_PROG_CC])
    AC_REQUIRE([AC_BAKEFILE_PROG_CXX])

    if test "x$BAKEFILE_HOST" = "x"; then
               if test "x${host}" = "x" ; then
                       AC_MSG_ERROR([You must call the autoconf "CANONICAL_HOST" macro in your configure.ac (or .in) file.])
               fi

        BAKEFILE_HOST="${host}"
    fi

    if test "x$BAKEFILE_CHECK_BASICS" != "xno"; then
        AC_BAKEFILE_CHECK_BASIC_STUFF
    fi
    AC_BAKEFILE_GNUMAKE
    AC_BAKEFILE_PLATFORM
    AC_BAKEFILE_PLATFORM_SPECIFICS
    AC_BAKEFILE_SUFFIXES
    AC_BAKEFILE_SHARED_LD
    AC_BAKEFILE_SHARED_VERSIONS
    AC_BAKEFILE_DEPS
    AC_BAKEFILE_RES_COMPILERS

    dnl OBJCFLAGS is set by Autoconf, but OBJCXXFLAGS is not:
    AC_SUBST(OBJCXXFLAGS)


    BAKEFILE_BAKEFILE_M4_VERSION="0.2.9"

    dnl includes autoconf_inc.m4:
    $1

    if test "$BAKEFILE_AUTOCONF_INC_M4_VERSION" = "" ; then
        AC_MSG_ERROR([No version found in autoconf_inc.m4 - bakefile macro was changed to take additional argument, perhaps configure.in wasn't updated (see the documentation)?])
    fi

    if test "$BAKEFILE_BAKEFILE_M4_VERSION" != "$BAKEFILE_AUTOCONF_INC_M4_VERSION" ; then
        AC_MSG_ERROR([Versions of Bakefile used to generate makefiles ($BAKEFILE_AUTOCONF_INC_M4_VERSION) and configure ($BAKEFILE_BAKEFILE_M4_VERSION) do not match.])
    fi
])


dnl ---------------------------------------------------------------------------
dnl              Embedded copies of helper scripts follow:
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_BAKEFILE_CREATE_FILE_BK_DEPS],
[
dnl ===================== bk-deps begins here =====================
dnl    (Created by merge-scripts.py from bk-deps
dnl     file do not edit here!)
D='$'
cat <<EOF >bk-deps
#!/bin/sh

# This script is part of Bakefile (http://www.bakefile.org) autoconf
# script. It is used to track C/C++ files dependencies in portable way.
#
# Permission is given to use this file in any way.

DEPSMODE=${DEPSMODE}
DEPSFLAG="${DEPSFLAG}"
DEPSDIRBASE=.deps

if test ${D}DEPSMODE = gcc ; then
    ${D}* ${D}{DEPSFLAG}
    status=${D}?

    # determine location of created files:
    while test ${D}# -gt 0; do
        case "${D}1" in
            -o )
                shift
                objfile=${D}1
            ;;
            -* )
            ;;
            * )
                srcfile=${D}1
            ;;
        esac
        shift
    done
    objfilebase=\`basename ${D}objfile\`
    builddir=\`dirname ${D}objfile\`
    depfile=\`basename ${D}srcfile | sed -e 's/\\..*${D}/.d/g'\`
    depobjname=\`echo ${D}depfile |sed -e 's/\\.d/.o/g'\`
    depsdir=${D}builddir/${D}DEPSDIRBASE
    mkdir -p ${D}depsdir

    # if the compiler failed, we're done:
    if test ${D}{status} != 0 ; then
        rm -f ${D}depfile
        exit ${D}{status}
    fi

    # move created file to the location we want it in:
    if test -f ${D}depfile ; then
        sed -e "s,${D}depobjname:,${D}objfile:,g" ${D}depfile >${D}{depsdir}/${D}{objfilebase}.d
        rm -f ${D}depfile
    else
        # "g++ -MMD -o fooobj.o foosrc.cpp" produces fooobj.d
        depfile=\`echo "${D}objfile" | sed -e 's/\\..*${D}/.d/g'\`
        if test ! -f ${D}depfile ; then
            # "cxx -MD -o fooobj.o foosrc.cpp" creates fooobj.o.d (Compaq C++)
            depfile="${D}objfile.d"
        fi
        if test -f ${D}depfile ; then
            sed -e "\\,^${D}objfile,!s,${D}depobjname:,${D}objfile:,g" ${D}depfile >${D}{depsdir}/${D}{objfilebase}.d
            rm -f ${D}depfile
        fi
    fi
    exit 0

elif test ${D}DEPSMODE = mwcc ; then
    ${D}* || exit ${D}?
    # Run mwcc again with -MM and redirect into the dep file we want
    # NOTE: We can't use shift here because we need ${D}* to be valid
    prevarg=
    for arg in ${D}* ; do
        if test "${D}prevarg" = "-o"; then
            objfile=${D}arg
        else
            case "${D}arg" in
                -* )
                ;;
                * )
                    srcfile=${D}arg
                ;;
            esac
        fi
        prevarg="${D}arg"
    done

    objfilebase=\`basename ${D}objfile\`
    builddir=\`dirname ${D}objfile\`
    depsdir=${D}builddir/${D}DEPSDIRBASE
    mkdir -p ${D}depsdir

    ${D}* ${D}DEPSFLAG >${D}{depsdir}/${D}{objfilebase}.d
    exit 0

elif test ${D}DEPSMODE = unixcc; then
    ${D}* || exit ${D}?
    # Run compiler again with deps flag and redirect into the dep file.
    # It doesn't work if the '-o FILE' option is used, but without it the
    # dependency file will contain the wrong name for the object. So it is
    # removed from the command line, and the dep file is fixed with sed.
    cmd=""
    while test ${D}# -gt 0; do
        case "${D}1" in
            -o )
                shift
                objfile=${D}1
            ;;
            * )
                eval arg${D}#=\\${D}1
                cmd="${D}cmd \\${D}arg${D}#"
            ;;
        esac
        shift
    done

    objfilebase=\`basename ${D}objfile\`
    builddir=\`dirname ${D}objfile\`
    depsdir=${D}builddir/${D}DEPSDIRBASE
    mkdir -p ${D}depsdir

    eval "${D}cmd ${D}DEPSFLAG" | sed "s|.*:|${D}objfile:|" >${D}{depsdir}/${D}{objfilebase}.d
    exit 0

else
    ${D}*
    exit ${D}?
fi
EOF
dnl ===================== bk-deps ends here =====================
])

AC_DEFUN([AC_BAKEFILE_CREATE_FILE_SHARED_LD_SH],
[
dnl ===================== shared-ld-sh begins here =====================
dnl    (Created by merge-scripts.py from shared-ld-sh
dnl     file do not edit here!)
D='$'
cat <<EOF >shared-ld-sh
#!/bin/sh
#-----------------------------------------------------------------------------
#-- Name:        distrib/mac/shared-ld-sh
#-- Purpose:     Link a mach-o dynamic shared library for Darwin / Mac OS X
#-- Author:      Gilles Depeyrot
#-- Copyright:   (c) 2002 Gilles Depeyrot
#-- Licence:     any use permitted
#-----------------------------------------------------------------------------

verbose=0
args=""
objects=""
linking_flag="-dynamiclib"
ldargs="-r -keep_private_externs -nostdlib"

if test "x${D}CXX" = "x"; then
    CXX="c++"
fi

while test ${D}# -gt 0; do
    case ${D}1 in

       -v)
        verbose=1
        ;;

       -o|-compatibility_version|-current_version|-framework|-undefined|-install_name)
        # collect these options and values
        args="${D}{args} ${D}1 ${D}2"
        shift
        ;;
       
       -arch|-isysroot)
        # collect these options and values
        ldargs="${D}{ldargs} ${D}1 ${D}2"
        shift
        ;;

       -s|-Wl,*)
        # collect these load args
        ldargs="${D}{ldargs} ${D}1"
        ;;

       -l*|-L*|-flat_namespace|-headerpad_max_install_names)
        # collect these options
        args="${D}{args} ${D}1"
        ;;

       -dynamiclib|-bundle)
        linking_flag="${D}1"
        ;;

       -*)
        echo "shared-ld: unhandled option '${D}1'"
        exit 1
        ;;

        *.o | *.a | *.dylib)
        # collect object files
        objects="${D}{objects} ${D}1"
        ;;

        *)
        echo "shared-ld: unhandled argument '${D}1'"
        exit 1
        ;;

    esac
    shift
done

status=0

#
# Link one module containing all the others
#
if test ${D}{verbose} = 1; then
    echo "${D}CXX ${D}{ldargs} ${D}{objects} -o master.${D}${D}.o"
fi
${D}CXX ${D}{ldargs} ${D}{objects} -o master.${D}${D}.o
status=${D}?

#
# Link the shared library from the single module created, but only if the
# previous command didn't fail:
#
if test ${D}{status} = 0; then
    if test ${D}{verbose} = 1; then
        echo "${D}CXX ${D}{linking_flag} master.${D}${D}.o ${D}{args}"
    fi
    ${D}CXX ${D}{linking_flag} master.${D}${D}.o ${D}{args}
    status=${D}?
fi

#
# Remove intermediate module
#
rm -f master.${D}${D}.o

exit ${D}status
EOF
dnl ===================== shared-ld-sh ends here =====================
])

AC_DEFUN([AC_BAKEFILE_CREATE_FILE_BK_MAKE_PCH],
[
dnl ===================== bk-make-pch begins here =====================
dnl    (Created by merge-scripts.py from bk-make-pch
dnl     file do not edit here!)
D='$'
cat <<EOF >bk-make-pch
#!/bin/sh

# This script is part of Bakefile (http://www.bakefile.org) autoconf
# script. It is used to generated precompiled headers.
#
# Permission is given to use this file in any way.

outfile="${D}{1}"
header="${D}{2}"
shift
shift

builddir=\`echo ${D}outfile | sed -e 's,/\\.pch/.*${D},,g'\`

compiler=""
headerfile=""

while test ${D}{#} -gt 0; do
    add_to_cmdline=1
    case "${D}{1}" in
        -I* )
            incdir=\`echo ${D}{1} | sed -e 's/-I\\(.*\\)/\\1/g'\`
            if test "x${D}{headerfile}" = "x" -a -f "${D}{incdir}/${D}{header}" ; then
                headerfile="${D}{incdir}/${D}{header}"
            fi
        ;;
        -use-pch|-use_pch|-pch-use )
            shift
            add_to_cmdline=0
        ;;
    esac
    if test ${D}add_to_cmdline = 1 ; then
        compiler="${D}{compiler} ${D}{1}"
    fi
    shift
done

if test "x${D}{headerfile}" = "x" ; then
    echo "error: can't find header ${D}{header} in include paths" >&2
else
    if test -f ${D}{outfile} ; then
        rm -f ${D}{outfile}
    else
        mkdir -p \`dirname ${D}{outfile}\`
    fi
    depsfile="${D}{builddir}/.deps/\`echo ${D}{outfile} | tr '/.' '__'\`.d"
    mkdir -p ${D}{builddir}/.deps
    if test "x${GCC_PCH}" = "x1" ; then
        # can do this because gcc is >= 3.4:
        ${D}{compiler} -o ${D}{outfile} -MMD -MF "${D}{depsfile}" "${D}{headerfile}"
    elif test "x${ICC_PCH}" = "x1" ; then
        filename=pch_gen-${D}${D}
        file=${D}{filename}.c
        dfile=${D}{filename}.d
        cat > ${D}file <<EOT
#include "${D}header"
EOT
        # using -MF icc complains about differing command lines in creation/use
        ${D}compiler -c ${ICC_PCH_CREATE_SWITCH} ${D}outfile -MMD ${D}file && \\
          sed -e "s,^.*:,${D}outfile:," -e "s, ${D}file,," < ${D}dfile > ${D}depsfile && \\
          rm -f ${D}file ${D}dfile ${D}{filename}.o
    fi
    exit ${D}{?}
fi
EOF
dnl ===================== bk-make-pch ends here =====================
])

dnl ---------------------------------------------------------------------------
dnl Author:          wxWidgets development team,
dnl                  Francesco Montorsi,
dnl                  Bob McCown (Mac-testing)
dnl Creation date:   24/11/2001
dnl ---------------------------------------------------------------------------

dnl ===========================================================================
dnl Table of Contents of this macro file:
dnl -------------------------------------
dnl
dnl SECTION A: wxWidgets main macros
dnl  - WX_CONFIG_OPTIONS
dnl  - WX_CONFIG_CHECK
dnl  - WXRC_CHECK
dnl  - WX_STANDARD_OPTIONS
dnl  - WX_CONVERT_STANDARD_OPTIONS_TO_WXCONFIG_FLAGS
dnl  - WX_DETECT_STANDARD_OPTION_VALUES
dnl
dnl SECTION B: wxWidgets-related utilities
dnl  - WX_LIKE_LIBNAME
dnl  - WX_ARG_ENABLE_YESNOAUTO
dnl  - WX_ARG_WITH_YESNOAUTO
dnl
dnl SECTION C: messages to the user
dnl  - WX_STANDARD_OPTIONS_SUMMARY_MSG
dnl  - WX_STANDARD_OPTIONS_SUMMARY_MSG_BEGIN
dnl  - WX_STANDARD_OPTIONS_SUMMARY_MSG_END
dnl  - WX_BOOLOPT_SUMMARY
dnl
dnl The special "WX_DEBUG_CONFIGURE" variable can be set to 1 to enable extra
dnl debug output on stdout from these macros.
dnl ===========================================================================


dnl ---------------------------------------------------------------------------
dnl Macros for wxWidgets detection. Typically used in configure.in as:
dnl
dnl     AC_ARG_ENABLE(...)
dnl     AC_ARG_WITH(...)
dnl        ...
dnl     WX_CONFIG_OPTIONS
dnl        ...
dnl        ...
dnl     WX_CONFIG_CHECK([2.6.0], [wxWin=1])
dnl     if test "$wxWin" != 1; then
dnl        AC_MSG_ERROR([
dnl                wxWidgets must be installed on your system
dnl                but wx-config script couldn't be found.
dnl
dnl                Please check that wx-config is in path, the directory
dnl                where wxWidgets libraries are installed (returned by
dnl                'wx-config --libs' command) is in LD_LIBRARY_PATH or
dnl                equivalent variable and wxWidgets version is 2.3.4 or above.
dnl        ])
dnl     fi
dnl     CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
dnl     CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
dnl     CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
dnl
dnl     LIBS="$LIBS $WX_LIBS"
dnl
dnl If you want to support standard --enable-debug/unicode/shared options, you
dnl may do the following:
dnl
dnl     ...
dnl     AC_CANONICAL_SYSTEM
dnl
dnl     # define configure options
dnl     WX_CONFIG_OPTIONS
dnl     WX_STANDARD_OPTIONS([debug,unicode,shared,toolkit,wxshared])
dnl
dnl     # basic configure checks
dnl     ...
dnl
dnl     # we want to always have DEBUG==WX_DEBUG and UNICODE==WX_UNICODE
dnl     WX_DEBUG=$DEBUG
dnl     WX_UNICODE=$UNICODE
dnl
dnl     WX_CONVERT_STANDARD_OPTIONS_TO_WXCONFIG_FLAGS
dnl     WX_CONFIG_CHECK([2.8.0], [wxWin=1],,[html,core,net,base],[$WXCONFIG_FLAGS])
dnl     WX_DETECT_STANDARD_OPTION_VALUES
dnl
dnl     # write the output files
dnl     AC_CONFIG_FILES([Makefile ...])
dnl     AC_OUTPUT
dnl
dnl     # optional: just to show a message to the user
dnl     WX_STANDARD_OPTIONS_SUMMARY_MSG
dnl
dnl ---------------------------------------------------------------------------


dnl ---------------------------------------------------------------------------
dnl WX_CONFIG_OPTIONS
dnl
dnl adds support for --wx-prefix, --wx-exec-prefix, --with-wxdir and
dnl --wx-config command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([WX_CONFIG_OPTIONS],
[
    AC_ARG_WITH(wxdir,
                [  --with-wxdir=PATH       Use uninstalled version of wxWidgets in PATH],
                [ wx_config_name="$withval/wx-config"
                  wx_config_args="--inplace"])
    AC_ARG_WITH(wx-config,
                [  --with-wx-config=CONFIG wx-config script to use (optional)],
                wx_config_name="$withval" )
    AC_ARG_WITH(wx-prefix,
                [  --with-wx-prefix=PREFIX Prefix where wxWidgets is installed (optional)],
                wx_config_prefix="$withval", wx_config_prefix="")
    AC_ARG_WITH(wx-exec-prefix,
                [  --with-wx-exec-prefix=PREFIX
                          Exec prefix where wxWidgets is installed (optional)],
                wx_config_exec_prefix="$withval", wx_config_exec_prefix="")
])

dnl Helper macro for checking if wx version is at least $1.$2.$3, set's
dnl wx_ver_ok=yes if it is:
AC_DEFUN([_WX_PRIVATE_CHECK_VERSION],
[
    wx_ver_ok=""
    if test "x$WX_VERSION" != x ; then
      if test $wx_config_major_version -gt $1; then
        wx_ver_ok=yes
      else
        if test $wx_config_major_version -eq $1; then
           if test $wx_config_minor_version -gt $2; then
              wx_ver_ok=yes
           else
              if test $wx_config_minor_version -eq $2; then
                 if test $wx_config_micro_version -ge $3; then
                    wx_ver_ok=yes
                 fi
              fi
           fi
        fi
      fi
    fi
])

dnl ---------------------------------------------------------------------------
dnl WX_CONFIG_CHECK(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl                  [, WX-LIBS [, ADDITIONAL-WX-CONFIG-FLAGS]]]])
dnl
dnl Test for wxWidgets, and define WX_C*FLAGS, WX_LIBS and WX_LIBS_STATIC
dnl (the latter is for static linking against wxWidgets). Set WX_CONFIG_NAME
dnl environment variable to override the default name of the wx-config script
dnl to use. Set WX_CONFIG_PATH to specify the full path to wx-config - in this
dnl case the macro won't even waste time on tests for its existence.
dnl
dnl Optional WX-LIBS argument contains comma- or space-separated list of
dnl wxWidgets libraries to link against. If it is not specified then WX_LIBS
dnl and WX_LIBS_STATIC will contain flags to link with all of the core
dnl wxWidgets libraries.
dnl
dnl Optional ADDITIONAL-WX-CONFIG-FLAGS argument is appended to wx-config
dnl invocation command in present. It can be used to fine-tune lookup of
dnl best wxWidgets build available.
dnl
dnl Example use:
dnl   WX_CONFIG_CHECK([2.6.0], [wxWin=1], [wxWin=0], [html,core,net]
dnl                    [--unicode --debug])
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the wx-config script
dnl
AC_DEFUN([WX_CONFIG_CHECK],
[
  dnl do we have wx-config name: it can be wx-config or wxd-config or ...
  if test x${WX_CONFIG_NAME+set} != xset ; then
     WX_CONFIG_NAME=wx-config
  fi

  if test "x$wx_config_name" != x ; then
     WX_CONFIG_NAME="$wx_config_name"
  fi

  dnl deal with optional prefixes
  if test x$wx_config_exec_prefix != x ; then
     wx_config_args="$wx_config_args --exec-prefix=$wx_config_exec_prefix"
     WX_LOOKUP_PATH="$wx_config_exec_prefix/bin"
  fi
  if test x$wx_config_prefix != x ; then
     wx_config_args="$wx_config_args --prefix=$wx_config_prefix"
     WX_LOOKUP_PATH="$WX_LOOKUP_PATH:$wx_config_prefix/bin"
  fi
  if test "$cross_compiling" = "yes"; then
     wx_config_args="$wx_config_args --host=$host_alias"
  fi

  dnl don't search the PATH if WX_CONFIG_NAME is absolute filename
  if test -x "$WX_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for wx-config)
     WX_CONFIG_PATH="$WX_CONFIG_NAME"
     AC_MSG_RESULT($WX_CONFIG_PATH)
  else
     AC_PATH_PROG(WX_CONFIG_PATH, $WX_CONFIG_NAME, no, "$WX_LOOKUP_PATH:$PATH")
  fi

  if test "$WX_CONFIG_PATH" != "no" ; then
    WX_VERSION=""

    min_wx_version=ifelse([$1], ,2.2.1,$1)
    if test -z "$5" ; then
      AC_MSG_CHECKING([for wxWidgets version >= $min_wx_version])
    else
      AC_MSG_CHECKING([for wxWidgets version >= $min_wx_version ($5)])
    fi

    dnl don't add the libraries ($4) to this variable as this would result in
    dnl an error when it's used with --version below
    WX_CONFIG_WITH_ARGS="$WX_CONFIG_PATH $wx_config_args $5"

    WX_VERSION=`$WX_CONFIG_WITH_ARGS --version 2>/dev/null`
    wx_config_major_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_config_minor_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_config_micro_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    wx_requested_major_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_requested_minor_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_requested_micro_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    _WX_PRIVATE_CHECK_VERSION([$wx_requested_major_version],
                              [$wx_requested_minor_version],
                              [$wx_requested_micro_version])

    if test -n "$wx_ver_ok"; then
      AC_MSG_RESULT(yes (version $WX_VERSION))
      WX_LIBS=`$WX_CONFIG_WITH_ARGS --libs $4`

      dnl is this even still appropriate?  --static is a real option now
      dnl and WX_CONFIG_WITH_ARGS is likely to contain it if that is
      dnl what the user actually wants, making this redundant at best.
      dnl For now keep it in case anyone actually used it in the past.
      AC_MSG_CHECKING([for wxWidgets static library])
      WX_LIBS_STATIC=`$WX_CONFIG_WITH_ARGS --static --libs $4 2>/dev/null`
      if test "x$WX_LIBS_STATIC" = "x"; then
        AC_MSG_RESULT(no)
      else
        AC_MSG_RESULT(yes)
      fi

      dnl starting with version 2.2.6 wx-config has --cppflags argument
      wx_has_cppflags=""
      if test $wx_config_major_version -gt 2; then
        wx_has_cppflags=yes
      else
        if test $wx_config_major_version -eq 2; then
           if test $wx_config_minor_version -gt 2; then
              wx_has_cppflags=yes
           else
              if test $wx_config_minor_version -eq 2; then
                 if test $wx_config_micro_version -ge 6; then
                    wx_has_cppflags=yes
                 fi
              fi
           fi
        fi
      fi

      dnl starting with version 2.7.0 wx-config has --rescomp option
      wx_has_rescomp=""
      if test $wx_config_major_version -gt 2; then
        wx_has_rescomp=yes
      else
        if test $wx_config_major_version -eq 2; then
           if test $wx_config_minor_version -ge 7; then
              wx_has_rescomp=yes
           fi
        fi
      fi
      if test "x$wx_has_rescomp" = x ; then
         dnl cannot give any useful info for resource compiler
         WX_RESCOMP=
      else
         WX_RESCOMP=`$WX_CONFIG_WITH_ARGS --rescomp`
      fi

      if test "x$wx_has_cppflags" = x ; then
         dnl no choice but to define all flags like CFLAGS
         WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags $4`
         WX_CPPFLAGS=$WX_CFLAGS
         WX_CXXFLAGS=$WX_CFLAGS

         WX_CFLAGS_ONLY=$WX_CFLAGS
         WX_CXXFLAGS_ONLY=$WX_CFLAGS
      else
         dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS
         WX_CPPFLAGS=`$WX_CONFIG_WITH_ARGS --cppflags $4`
         WX_CXXFLAGS=`$WX_CONFIG_WITH_ARGS --cxxflags $4`
         WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags $4`

         WX_CFLAGS_ONLY=`echo $WX_CFLAGS | sed "s@^$WX_CPPFLAGS *@@"`
         WX_CXXFLAGS_ONLY=`echo $WX_CXXFLAGS | sed "s@^$WX_CFLAGS *@@"`
      fi

      ifelse([$2], , :, [$2])

    else

       if test "x$WX_VERSION" = x; then
          dnl no wx-config at all
          AC_MSG_RESULT(no)
       else
          AC_MSG_RESULT(no (version $WX_VERSION is not new enough))
       fi

       WX_CFLAGS=""
       WX_CPPFLAGS=""
       WX_CXXFLAGS=""
       WX_LIBS=""
       WX_LIBS_STATIC=""
       WX_RESCOMP=""

       if test ! -z "$5"; then

          wx_error_message="
    The configuration you asked for $PACKAGE_NAME requires a wxWidgets
    build with the following settings:
        $5
    but such build is not available.

    To see the wxWidgets builds available on this system, please use
    'wx-config --list' command. To use the default build, returned by
    'wx-config --selected-config', use the options with their 'auto'
    default values."

       fi

       wx_error_message="
    The requested wxWidgets build couldn't be found.
    $wx_error_message

    If you still get this error, then check that 'wx-config' is
    in path, the directory where wxWidgets libraries are installed
    (returned by 'wx-config --libs' command) is in LD_LIBRARY_PATH
    or equivalent variable and wxWidgets version is $1 or above."

       ifelse([$3], , AC_MSG_ERROR([$wx_error_message]), [$3])

    fi
  else

    WX_CFLAGS=""
    WX_CPPFLAGS=""
    WX_CXXFLAGS=""
    WX_LIBS=""
    WX_LIBS_STATIC=""
    WX_RESCOMP=""

    ifelse([$3], , :, [$3])

  fi

  AC_SUBST(WX_CPPFLAGS)
  AC_SUBST(WX_CFLAGS)
  AC_SUBST(WX_CXXFLAGS)
  AC_SUBST(WX_CFLAGS_ONLY)
  AC_SUBST(WX_CXXFLAGS_ONLY)
  AC_SUBST(WX_LIBS)
  AC_SUBST(WX_LIBS_STATIC)
  AC_SUBST(WX_VERSION)
  AC_SUBST(WX_RESCOMP)

  dnl need to export also WX_VERSION_MINOR and WX_VERSION_MAJOR symbols
  dnl to support wxpresets bakefiles (we export also WX_VERSION_MICRO for completeness):
  WX_VERSION_MAJOR="$wx_config_major_version"
  WX_VERSION_MINOR="$wx_config_minor_version"
  WX_VERSION_MICRO="$wx_config_micro_version"
  AC_SUBST(WX_VERSION_MAJOR)
  AC_SUBST(WX_VERSION_MINOR)
  AC_SUBST(WX_VERSION_MICRO)
])

dnl ---------------------------------------------------------------------------
dnl Get information on the wxrc program for making C++, Python and xrs
dnl resource files.
dnl
dnl     AC_ARG_ENABLE(...)
dnl     AC_ARG_WITH(...)
dnl        ...
dnl     WX_CONFIG_OPTIONS
dnl        ...
dnl     WX_CONFIG_CHECK(2.6.0, wxWin=1)
dnl     if test "$wxWin" != 1; then
dnl        AC_MSG_ERROR([
dnl                wxWidgets must be installed on your system
dnl                but wx-config script couldn't be found.
dnl
dnl                Please check that wx-config is in path, the directory
dnl                where wxWidgets libraries are installed (returned by
dnl                'wx-config --libs' command) is in LD_LIBRARY_PATH or
dnl                equivalent variable and wxWidgets version is 2.6.0 or above.
dnl        ])
dnl     fi
dnl
dnl     WXRC_CHECK([HAVE_WXRC=1], [HAVE_WXRC=0])
dnl     if test "x$HAVE_WXRC" != x1; then
dnl         AC_MSG_ERROR([
dnl                The wxrc program was not installed or not found.
dnl
dnl                Please check the wxWidgets installation.
dnl         ])
dnl     fi
dnl
dnl     CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
dnl     CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
dnl     CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
dnl
dnl     LDFLAGS="$LDFLAGS $WX_LIBS"
dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl WXRC_CHECK([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for wxWidgets' wxrc program for creating either C++, Python or XRS
dnl resources.  The variable WXRC will be set and substituted in the configure
dnl script and Makefiles.
dnl
dnl Example use:
dnl   WXRC_CHECK([wxrc=1], [wxrc=0])
dnl ---------------------------------------------------------------------------

dnl
dnl wxrc program from the wx-config script
dnl
AC_DEFUN([WXRC_CHECK],
[
  AC_ARG_VAR([WXRC], [Path to wxWidget's wxrc resource compiler])

  if test "x$WX_CONFIG_NAME" = x; then
    AC_MSG_ERROR([The wxrc tests must run after wxWidgets test.])
  else

    AC_MSG_CHECKING([for wxrc])

    if test "x$WXRC" = x ; then
      dnl wx-config --utility is a new addition to wxWidgets:
      _WX_PRIVATE_CHECK_VERSION(2,5,3)
      if test -n "$wx_ver_ok"; then
        WXRC=`$WX_CONFIG_WITH_ARGS --utility=wxrc`
      fi
    fi

    if test "x$WXRC" = x ; then
      AC_MSG_RESULT([not found])
      ifelse([$2], , :, [$2])
    else
      AC_MSG_RESULT([$WXRC])
      ifelse([$1], , :, [$1])
    fi

    AC_SUBST(WXRC)
  fi
])

dnl ---------------------------------------------------------------------------
dnl WX_LIKE_LIBNAME([output-var] [prefix], [name])
dnl
dnl Sets the "output-var" variable to the name of a library named with same
dnl wxWidgets rule.
dnl E.g. for output-var=='lib', name=='test', prefix='mine', sets
dnl      the $lib variable to:
dnl          'mine_gtk2ud_test-2.8'
dnl      if WX_PORT=gtk2, WX_UNICODE=1, WX_DEBUG=1 and WX_RELEASE=28
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_LIKE_LIBNAME],
    [
        wx_temp="$2""_""$WX_PORT"

        dnl add the [u][d] string
        if test "$WX_UNICODE" = "1"; then
            wx_temp="$wx_temp""u"
        fi
        if test "$WX_DEBUG" = "1"; then
            wx_temp="$wx_temp""d"
        fi

        dnl complete the name of the lib
        wx_temp="$wx_temp""_""$3""-$WX_VERSION_MAJOR.$WX_VERSION_MINOR"

        dnl save it in the user's variable
        $1=$wx_temp
    ])

dnl ---------------------------------------------------------------------------
dnl WX_ARG_ENABLE_YESNOAUTO/WX_ARG_WITH_YESNOAUTO
dnl
dnl Two little custom macros which define the ENABLE/WITH configure arguments.
dnl Macro arguments:
dnl $1 = the name of the --enable / --with  feature
dnl $2 = the name of the variable associated
dnl $3 = the description of that feature
dnl $4 = the default value for that feature
dnl $5 = additional action to do in case option is given with "yes" value
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_ARG_ENABLE_YESNOAUTO],
         [AC_ARG_ENABLE($1,
            AC_HELP_STRING([--enable-$1], [$3 (default is $4)]),
            [], [enableval="$4"])

            dnl Show a message to the user about this option
            AC_MSG_CHECKING([for the --enable-$1 option])
            if test "$enableval" = "yes" ; then
                AC_MSG_RESULT([yes])
                $2=1
                $5
            elif test "$enableval" = "no" ; then
                AC_MSG_RESULT([no])
                $2=0
            elif test "$enableval" = "auto" ; then
                AC_MSG_RESULT([will be automatically detected])
                $2="auto"
            else
                AC_MSG_ERROR([
    Unrecognized option value (allowed values: yes, no, auto)
                ])
            fi
         ])

AC_DEFUN([WX_ARG_WITH_YESNOAUTO],
         [AC_ARG_WITH($1,
            AC_HELP_STRING([--with-$1], [$3 (default is $4)]),
            [], [withval="$4"])

            dnl Show a message to the user about this option
            AC_MSG_CHECKING([for the --with-$1 option])
            if test "$withval" = "yes" ; then
                AC_MSG_RESULT([yes])
                $2=1
                $5
            dnl NB: by default we don't allow --with-$1=no option
            dnl     since it does not make much sense !
            elif test "$6" = "1" -a "$withval" = "no" ; then
                AC_MSG_RESULT([no])
                $2=0
            elif test "$withval" = "auto" ; then
                AC_MSG_RESULT([will be automatically detected])
                $2="auto"
            else
                AC_MSG_ERROR([
    Unrecognized option value (allowed values: yes, auto)
                ])
            fi
         ])


dnl ---------------------------------------------------------------------------
dnl WX_STANDARD_OPTIONS([options-to-add])
dnl
dnl Adds to the configure script one or more of the following options:
dnl   --enable-[debug|unicode|shared|wxshared|wxdebug]
dnl   --with-[gtk|msw|motif|x11|mac|dfb]
dnl   --with-wxversion
dnl Then checks for their presence and eventually set the DEBUG, UNICODE, SHARED,
dnl PORT, WX_SHARED, WX_DEBUG, variables to one of the "yes", "no", "auto" values.
dnl
dnl Note that e.g. UNICODE != WX_UNICODE; the first is the value of the
dnl --enable-unicode option (in boolean format) while the second indicates
dnl if wxWidgets was built in Unicode mode (and still is in boolean format).
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_STANDARD_OPTIONS],
        [

        dnl the following lines will expand to WX_ARG_ENABLE_YESNOAUTO calls if and only if
        dnl the $1 argument contains respectively the debug,unicode or shared options.

        dnl be careful here not to set debug flag if only "wxdebug" was specified
        ifelse(regexp([$1], [\bdebug]), [-1],,
               [WX_ARG_ENABLE_YESNOAUTO([debug], [DEBUG], [Build in debug mode], [auto])])

        ifelse(index([$1], [unicode]), [-1],,
               [WX_ARG_ENABLE_YESNOAUTO([unicode], [UNICODE], [Build in Unicode mode], [auto])])

        ifelse(regexp([$1], [\bshared]), [-1],,
               [WX_ARG_ENABLE_YESNOAUTO([shared], [SHARED], [Build as shared library], [auto])])

        dnl WX_ARG_WITH_YESNOAUTO cannot be used for --with-toolkit since it's an option
        dnl which must be able to accept the auto|gtk1|gtk2|msw|... values
        ifelse(index([$1], [toolkit]), [-1],,
               [
                AC_ARG_WITH([toolkit],
                            AC_HELP_STRING([--with-toolkit],
                                           [Build against a specific wxWidgets toolkit (default is auto)]),
                            [], [withval="auto"])

                dnl Show a message to the user about this option
                AC_MSG_CHECKING([for the --with-toolkit option])
                if test "$withval" = "auto" ; then
                    AC_MSG_RESULT([will be automatically detected])
                    TOOLKIT="auto"
                else
                    TOOLKIT="$withval"

                    dnl PORT must be one of the allowed values
                    if test "$TOOLKIT" != "gtk1" -a "$TOOLKIT" != "gtk2" -a \
                            "$TOOLKIT" != "msw" -a "$TOOLKIT" != "motif" -a \
                            "$TOOLKIT" != "osx_carbon" -a "$TOOLKIT" != "osx_cocoa" -a \
                            "$TOOLKIT" != "dfb" -a "$TOOLKIT" != "x11"; then
                        AC_MSG_ERROR([
    Unrecognized option value (allowed values: auto, gtk1, gtk2, msw, motif, osx_carbon, osx_cocoa, dfb, x11)
                        ])
                    fi

                    AC_MSG_RESULT([$TOOLKIT])
                fi
               ])

        dnl ****** IMPORTANT *******
        dnl   Unlike for the UNICODE setting, you can build your program in
        dnl   shared mode against a static build of wxWidgets. Thus we have the
        dnl   following option which allows these mixtures. E.g.
        dnl
        dnl      ./configure --disable-shared --with-wxshared
        dnl
        dnl   will build your library in static mode against the first available
        dnl   shared build of wxWidgets.
        dnl
        dnl   Note that's not possible to do the viceversa:
        dnl
        dnl      ./configure --enable-shared --without-wxshared
        dnl
        dnl   Doing so you would try to build your library in shared mode against a static
        dnl   build of wxWidgets. This is not possible (you would mix PIC and non PIC code) !
        dnl   A check for this combination of options is in WX_DETECT_STANDARD_OPTION_VALUES
        dnl   (where we know what 'auto' should be expanded to).
        dnl
        dnl   If you try to build something in ANSI mode against a UNICODE build
        dnl   of wxWidgets or in RELEASE mode against a DEBUG build of wxWidgets,
        dnl   then at best you'll get ton of linking errors !
        dnl ************************

        ifelse(index([$1], [wxshared]), [-1],,
               [
                WX_ARG_WITH_YESNOAUTO(
                    [wxshared], [WX_SHARED],
                    [Force building against a shared build of wxWidgets, even if --disable-shared is given],
                    [auto], [], [1])
               ])

        dnl Just like for SHARED and WX_SHARED it may happen that some adventurous
        dnl peoples will want to mix a wxWidgets release build with a debug build of
        dnl his app/lib. So, we have both DEBUG and WX_DEBUG variables.
        ifelse(index([$1], [wxdebug]), [-1],,
               [
                WX_ARG_WITH_YESNOAUTO(
                    [wxdebug], [WX_DEBUG],
                    [Force building against a debug build of wxWidgets, even if --disable-debug is given],
                    [auto], [], [1])
               ])

        dnl WX_ARG_WITH_YESNOAUTO cannot be used for --with-wxversion since it's an option
        dnl which accepts the "auto|2.6|2.7|2.8|2.9|3.0" etc etc values
        ifelse(index([$1], [wxversion]), [-1],,
               [
                AC_ARG_WITH([wxversion],
                            AC_HELP_STRING([--with-wxversion],
                                           [Build against a specific version of wxWidgets (default is auto)]),
                            [], [withval="auto"])

                dnl Show a message to the user about this option
                AC_MSG_CHECKING([for the --with-wxversion option])
                if test "$withval" = "auto" ; then
                    AC_MSG_RESULT([will be automatically detected])
                    WX_RELEASE="auto"
                else

                    wx_requested_major_version=`echo $withval | \
                        sed 's/\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
                    wx_requested_minor_version=`echo $withval | \
                        sed 's/\([[0-9]]*\).\([[0-9]]*\).*/\2/'`

                    dnl both vars above must be exactly 1 digit
                    if test "${#wx_requested_major_version}" != "1" -o \
                            "${#wx_requested_minor_version}" != "1" ; then
                        AC_MSG_ERROR([
    Unrecognized option value (allowed values: auto, 2.6, 2.7, 2.8, 2.9, 3.0)
                        ])
                    fi

                    WX_RELEASE="$wx_requested_major_version"".""$wx_requested_minor_version"
                    AC_MSG_RESULT([$WX_RELEASE])
                fi
               ])

        if test "$WX_DEBUG_CONFIGURE" = "1"; then
            echo "[[dbg]] DEBUG: $DEBUG, WX_DEBUG: $WX_DEBUG"
            echo "[[dbg]] UNICODE: $UNICODE, WX_UNICODE: $WX_UNICODE"
            echo "[[dbg]] SHARED: $SHARED, WX_SHARED: $WX_SHARED"
            echo "[[dbg]] TOOLKIT: $TOOLKIT, WX_TOOLKIT: $WX_TOOLKIT"
            echo "[[dbg]] VERSION: $VERSION, WX_RELEASE: $WX_RELEASE"
        fi
    ])


dnl ---------------------------------------------------------------------------
dnl WX_CONVERT_STANDARD_OPTIONS_TO_WXCONFIG_FLAGS
dnl
dnl Sets the WXCONFIG_FLAGS string using the SHARED,DEBUG,UNICODE variable values
dnl which are different from "auto".
dnl Thus this macro needs to be called only once all options have been set.
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_CONVERT_STANDARD_OPTIONS_TO_WXCONFIG_FLAGS],
        [
        if test "$WX_SHARED" = "1" ; then
            WXCONFIG_FLAGS="--static=no "
        elif test "$WX_SHARED" = "0" ; then
            WXCONFIG_FLAGS="--static=yes "
        fi

        if test "$WX_DEBUG" = "1" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--debug=yes "
        elif test "$WX_DEBUG" = "0" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--debug=no "
        fi

        dnl The user should have set WX_UNICODE=UNICODE
        if test "$WX_UNICODE" = "1" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--unicode=yes "
        elif test "$WX_UNICODE" = "0" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--unicode=no "
        fi

        if test "$TOOLKIT" != "auto" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--toolkit=$TOOLKIT "
        fi

        if test "$WX_RELEASE" != "auto" ; then
            WXCONFIG_FLAGS="$WXCONFIG_FLAGS""--version=$WX_RELEASE "
        fi

        dnl strip out the last space of the string
        WXCONFIG_FLAGS=${WXCONFIG_FLAGS% }

        if test "$WX_DEBUG_CONFIGURE" = "1"; then
            echo "[[dbg]] WXCONFIG_FLAGS: $WXCONFIG_FLAGS"
        fi
    ])


dnl ---------------------------------------------------------------------------
dnl _WX_SELECTEDCONFIG_CHECKFOR([RESULTVAR], [STRING], [MSG]
dnl                             [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Outputs the given MSG. Then searches the given STRING in the wxWidgets
dnl additional CPP flags and put the result of the search in WX_$RESULTVAR
dnl also adding the "yes" or "no" message result to MSG.
dnl ---------------------------------------------------------------------------
AC_DEFUN([_WX_SELECTEDCONFIG_CHECKFOR],
        [
        if test "$$1" = "auto" ; then

            dnl The user does not have particular preferences for this option;
            dnl so we will detect the wxWidgets relative build setting and use it
            AC_MSG_CHECKING([$3])

            dnl set WX_$1 variable to 1 if the $WX_SELECTEDCONFIG contains the $2
            dnl string or to 0 otherwise.
            dnl NOTE: 'expr match STRING REGEXP' cannot be used since on Mac it
            dnl       doesn't work; we use 'expr STRING : REGEXP' instead
            WX_$1=$(expr "$WX_SELECTEDCONFIG" : ".*$2.*")

            if test "$WX_$1" != "0"; then
                WX_$1=1
                AC_MSG_RESULT([yes])
                ifelse([$4], , :, [$4])
            else
                WX_$1=0
                AC_MSG_RESULT([no])
                ifelse([$5], , :, [$5])
            fi
        else

            dnl Use the setting given by the user
            WX_$1=$$1
        fi
    ])

dnl ---------------------------------------------------------------------------
dnl WX_DETECT_STANDARD_OPTION_VALUES
dnl
dnl Detects the values of the following variables:
dnl 1) WX_RELEASE
dnl 2) WX_UNICODE
dnl 3) WX_DEBUG
dnl 4) WX_SHARED    (and also WX_STATIC)
dnl 5) WX_PORT
dnl from the previously selected wxWidgets build; this macro in fact must be
dnl called *after* calling the WX_CONFIG_CHECK macro.
dnl
dnl Note that the WX_VERSION_MAJOR, WX_VERSION_MINOR symbols are already set
dnl by WX_CONFIG_CHECK macro
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_DETECT_STANDARD_OPTION_VALUES],
        [
        dnl IMPORTANT: WX_VERSION contains all three major.minor.micro digits,
        dnl            while WX_RELEASE only the major.minor ones.
        WX_RELEASE="$WX_VERSION_MAJOR""$WX_VERSION_MINOR"
        if test $WX_RELEASE -lt 26 ; then

            AC_MSG_ERROR([
    Cannot detect the wxWidgets configuration for the selected wxWidgets build
    since its version is $WX_VERSION < 2.6.0; please install a newer
    version of wxWidgets.
                         ])
        fi

        dnl The wx-config we are using understands the "--selected_config"
        dnl option which returns an easy-parseable string !
        WX_SELECTEDCONFIG=$($WX_CONFIG_WITH_ARGS --selected_config)

        if test "$WX_DEBUG_CONFIGURE" = "1"; then
            echo "[[dbg]] Using wx-config --selected-config"
            echo "[[dbg]] WX_SELECTEDCONFIG: $WX_SELECTEDCONFIG"
        fi


        dnl we could test directly for WX_SHARED with a line like:
        dnl    _WX_SELECTEDCONFIG_CHECKFOR([SHARED], [shared],
        dnl                                [if wxWidgets was built in SHARED mode])
        dnl but wx-config --selected-config DOES NOT outputs the 'shared'
        dnl word when wx was built in shared mode; it rather outputs the
        dnl 'static' word when built in static mode.
        if test $WX_SHARED = "1"; then
            STATIC=0
        elif test $WX_SHARED = "0"; then
            STATIC=1
        elif test $WX_SHARED = "auto"; then
            STATIC="auto"
        fi

        dnl Now set the WX_UNICODE, WX_DEBUG, WX_STATIC variables
        _WX_SELECTEDCONFIG_CHECKFOR([UNICODE], [unicode],
                                    [if wxWidgets was built with UNICODE enabled])
        _WX_SELECTEDCONFIG_CHECKFOR([DEBUG], [debug],
                                    [if wxWidgets was built in DEBUG mode])
        _WX_SELECTEDCONFIG_CHECKFOR([STATIC], [static],
                                    [if wxWidgets was built in STATIC mode])

        dnl init WX_SHARED from WX_STATIC
        if test "$WX_STATIC" != "0"; then
            WX_SHARED=0
        else
            WX_SHARED=1
        fi

        AC_SUBST(WX_UNICODE)
        AC_SUBST(WX_DEBUG)
        AC_SUBST(WX_SHARED)

        dnl detect the WX_PORT to use
        if test "$TOOLKIT" = "auto" ; then

            dnl The user does not have particular preferences for this option;
            dnl so we will detect the wxWidgets relative build setting and use it
            AC_MSG_CHECKING([which wxWidgets toolkit was selected])

            WX_GTKPORT1=$(expr "$WX_SELECTEDCONFIG" : ".*gtk1.*")
            WX_GTKPORT2=$(expr "$WX_SELECTEDCONFIG" : ".*gtk2.*")
            WX_MSWPORT=$(expr "$WX_SELECTEDCONFIG" : ".*msw.*")
            WX_MOTIFPORT=$(expr "$WX_SELECTEDCONFIG" : ".*motif.*")
            WX_OSXCOCOAPORT=$(expr "$WX_SELECTEDCONFIG" : ".*osx_cocoa.*")
            WX_OSXCARBONPORT=$(expr "$WX_SELECTEDCONFIG" : ".*osx_carbon.*")
            WX_X11PORT=$(expr "$WX_SELECTEDCONFIG" : ".*x11.*")
            WX_DFBPORT=$(expr "$WX_SELECTEDCONFIG" : ".*dfb.*")

            WX_PORT="unknown"
            if test "$WX_GTKPORT1" != "0"; then WX_PORT="gtk1"; fi
            if test "$WX_GTKPORT2" != "0"; then WX_PORT="gtk2"; fi
            if test "$WX_MSWPORT" != "0"; then WX_PORT="msw"; fi
            if test "$WX_MOTIFPORT" != "0"; then WX_PORT="motif"; fi
            if test "$WX_OSXCOCOAPORT" != "0"; then WX_PORT="osx_cocoa"; fi
            if test "$WX_OSXCARBONPORT" != "0"; then WX_PORT="osx_carbon"; fi
            if test "$WX_X11PORT" != "0"; then WX_PORT="x11"; fi
            if test "$WX_DFBPORT" != "0"; then WX_PORT="dfb"; fi

            dnl NOTE: backward-compatible check for wx2.8; in wx2.9 the mac
            dnl       ports are called 'osx_cocoa' and 'osx_carbon' (see above)
            WX_MACPORT=$(expr "$WX_SELECTEDCONFIG" : ".*mac.*")
            if test "$WX_MACPORT" != "0"; then WX_PORT="mac"; fi

            dnl check at least one of the WX_*PORT has been set !

            if test "$WX_PORT" = "unknown" ; then
                AC_MSG_ERROR([
        Cannot detect the currently installed wxWidgets port !
        Please check your 'wx-config --cxxflags'...
                            ])
            fi

            AC_MSG_RESULT([$WX_PORT])
        else

            dnl Use the setting given by the user
            if test -z "$TOOLKIT" ; then
                WX_PORT=$TOOLKIT
            else
                dnl try with PORT
                WX_PORT=$PORT
            fi
        fi

        AC_SUBST(WX_PORT)

        if test "$WX_DEBUG_CONFIGURE" = "1"; then
            echo "[[dbg]] Values of all WX_* options after final detection:"
            echo "[[dbg]] WX_DEBUG: $WX_DEBUG"
            echo "[[dbg]] WX_UNICODE: $WX_UNICODE"
            echo "[[dbg]] WX_SHARED: $WX_SHARED"
            echo "[[dbg]] WX_RELEASE: $WX_RELEASE"
            echo "[[dbg]] WX_PORT: $WX_PORT"
        fi

        dnl Avoid problem described in the WX_STANDARD_OPTIONS which happens when
        dnl the user gives the options:
        dnl      ./configure --enable-shared --without-wxshared
        dnl or just do
        dnl      ./configure --enable-shared
        dnl but there is only a static build of wxWidgets available.
        if test "$WX_SHARED" = "0" -a "$SHARED" = "1"; then
            AC_MSG_ERROR([
    Cannot build shared library against a static build of wxWidgets !
    This error happens because the wxWidgets build which was selected
    has been detected as static while you asked to build $PACKAGE_NAME
    as shared library and this is not possible.
    Use the '--disable-shared' option to build $PACKAGE_NAME
    as static library or '--with-wxshared' to use wxWidgets as shared library.
                         ])
        fi

        dnl now we can finally update the DEBUG,UNICODE,SHARED options
        dnl to their final values if they were set to 'auto'
        if test "$DEBUG" = "auto"; then
            DEBUG=$WX_DEBUG
        fi
        if test "$UNICODE" = "auto"; then
            UNICODE=$WX_UNICODE
        fi
        if test "$SHARED" = "auto"; then
            SHARED=$WX_SHARED
        fi
        if test "$TOOLKIT" = "auto"; then
            TOOLKIT=$WX_PORT
        fi

        dnl in case the user needs a BUILD=debug/release var...
        if test "$DEBUG" = "1"; then
            BUILD="debug"
        elif test "$DEBUG" = "0" -o "$DEBUG" = ""; then
            BUILD="release"
        fi

        dnl respect the DEBUG variable adding the optimize/debug flags
        dnl NOTE: the CXXFLAGS are merged together with the CPPFLAGS so we
        dnl       don't need to set them, too
        if test "$DEBUG" = "1"; then
            CXXFLAGS="$CXXFLAGS -g -O0"
            CFLAGS="$CFLAGS -g -O0"
        else
            CXXFLAGS="$CXXFLAGS -O2"
            CFLAGS="$CFLAGS -O2"
        fi
    ])

dnl ---------------------------------------------------------------------------
dnl WX_BOOLOPT_SUMMARY([name of the boolean variable to show summary for],
dnl                   [what to print when var is 1],
dnl                   [what to print when var is 0])
dnl
dnl Prints $2 when variable $1 == 1 and prints $3 when variable $1 == 0.
dnl This macro mainly exists just to make configure.ac scripts more readable.
dnl
dnl NOTE: you need to use the [" my message"] syntax for 2nd and 3rd arguments
dnl       if you want that m4 avoid to throw away the spaces prefixed to the
dnl       argument value.
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_BOOLOPT_SUMMARY],
        [
        if test "x$$1" = "x1" ; then
            echo $2
        elif test "x$$1" = "x0" ; then
            echo $3
        else
            echo "$1 is $$1"
        fi
    ])

dnl ---------------------------------------------------------------------------
dnl WX_STANDARD_OPTIONS_SUMMARY_MSG
dnl
dnl Shows a summary message to the user about the WX_* variable contents.
dnl This macro is used typically at the end of the configure script.
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_STANDARD_OPTIONS_SUMMARY_MSG],
        [
        echo
        echo "  The wxWidgets build which will be used by $PACKAGE_NAME $PACKAGE_VERSION"
        echo "  has the following settings:"
        WX_BOOLOPT_SUMMARY([WX_DEBUG],   ["  - DEBUG build"],  ["  - RELEASE build"])
        WX_BOOLOPT_SUMMARY([WX_UNICODE], ["  - UNICODE mode"], ["  - ANSI mode"])
        WX_BOOLOPT_SUMMARY([WX_SHARED],  ["  - SHARED mode"],  ["  - STATIC mode"])
        echo "  - VERSION: $WX_VERSION"
        echo "  - PORT: $WX_PORT"
    ])


dnl ---------------------------------------------------------------------------
dnl WX_STANDARD_OPTIONS_SUMMARY_MSG_BEGIN, WX_STANDARD_OPTIONS_SUMMARY_MSG_END
dnl
dnl Like WX_STANDARD_OPTIONS_SUMMARY_MSG macro but these two macros also gives info
dnl about the configuration of the package which used the wxpresets.
dnl
dnl Typical usage:
dnl    WX_STANDARD_OPTIONS_SUMMARY_MSG_BEGIN
dnl    echo "   - Package setting 1: $SETTING1"
dnl    echo "   - Package setting 2: $SETTING1"
dnl    ...
dnl    WX_STANDARD_OPTIONS_SUMMARY_MSG_END
dnl
dnl ---------------------------------------------------------------------------
AC_DEFUN([WX_STANDARD_OPTIONS_SUMMARY_MSG_BEGIN],
        [
        echo
        echo " ----------------------------------------------------------------"
        echo "  Configuration for $PACKAGE_NAME $PACKAGE_VERSION successfully completed."
        echo "  Summary of main configuration settings for $PACKAGE_NAME:"
        WX_BOOLOPT_SUMMARY([DEBUG], ["  - DEBUG build"], ["  - RELEASE build"])
        WX_BOOLOPT_SUMMARY([UNICODE], ["  - UNICODE mode"], ["  - ANSI mode"])
        WX_BOOLOPT_SUMMARY([SHARED], ["  - SHARED mode"], ["  - STATIC mode"])
    ])

AC_DEFUN([WX_STANDARD_OPTIONS_SUMMARY_MSG_END],
        [
        WX_STANDARD_OPTIONS_SUMMARY_MSG
        echo
        echo "  Now, just run make."
        echo " ----------------------------------------------------------------"
        echo
    ])


dnl ---------------------------------------------------------------------------
dnl Deprecated macro wrappers
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_WXCONFIG], [WX_CONFIG_OPTIONS])
AC_DEFUN([AM_PATH_WXCONFIG], [
    WX_CONFIG_CHECK([$1],[$2],[$3],[$4],[$5])
])
AC_DEFUN([AM_PATH_WXRC], [WXRC_CHECK([$1],[$2])])

m4_include([m4/boost.m4])
m4_include([m4/libtool.m4])
m4_include([m4/ltoptions.m4])
m4_include([m4/ltsugar.m4])
m4_include([m4/ltversion.m4])
m4_include([m4/lt~obsolete.m4])
