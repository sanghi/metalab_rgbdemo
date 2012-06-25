#!/bin/sh
##
## compile_linux.sh
## 
## 
## Copyright (C) 2012 MetaLAB at Berkman Center for Internet & Society at Harvard University
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##

dump_ansi_colors ()
{
    for attr in 0 1 4 5 7 ; do
        echo "----------------------------------------------------------------"
        printf "ESC[%s;Foreground;Background - \n" $attr
        for fore in 30 31 32 33 34 35 36 37; do
            for back in 40 41 42 43 44 45 46 47; do
                printf '\033[%s;%s;%sm %02s;%02s  ' $attr $fore $back $fore $back
            done
        printf '\n'
        done
        printf '\033[0m'
    done
}

ansi_red()
{
    printf '\033[1;31;40m'
}

ansi_purple()
{
    printf '\033[1;34;40m'
}

ansi_clear()
{
    printf '\033[0m'
}

try_run ()
{
    echo `ansi_purple` "[RUN]" `ansi_clear` " $@"
    if ! "$@"; then
        echo `ansi_red` "[ERR]" `ansi_clear` " $@"
    fi
}

run ()
{
    echo `ansi_purple` "[RUN]" `ansi_clear` " $@"
    if ! "$@"; then
        echo `ansi_red` "[ERR]" `ansi_clear` " $@"
        exit 1
    fi
}

dbg_vars () # name
{
    for name in $@; do
        echo "[DBG] $name=\"${!name}\""
    done
}

#-------------------------------------------------------------------------------

for app in $@;
do
    program=`echo $app | sed -e s/\.app//`
    macdeployqt $app

    bin=`basename $program`
    cd $app/Contents/Frameworks

    rm -rf Qt*.framework

    for l in ../../../common/lib/*.dylib ../../../common/*.framework; do
	ln -sf $l .
    done

    for lib in *.dylib; do
	for libpath in \
	    "$lib" \
	    "lib/$lib" \
	    "@executable_path/../MacOS/$lib" \
	    "/opt/local/lib/$lib" \
	    "/usr/local/lib/$lib" \
	    "/usr/lib/$lib" \
	    "/usr/X11/lib/$lib"; do
	    try_run install_name_tool -change "$libpath" @executable_path/../Frameworks/$lib ../MacOS/$bin
	done
    done

# If not already done, uncomment this.
if false; then
    for bin in *.dylib; do
	for QTLIB in QtCore QtOpenGL QtSvg QtXml QtNetwork QtGui QtTest; do
	    try_run install_name_tool -change ${QTLIB}.framework/Versions/4/${QTLIB} \
		@executable_path/../Frameworks/${QTLIB}.framework/Versions/4/${QTLIB} $bin
	done

	try_run install_name_tool -id @executable_path/../Frameworks/$bin $bin

	try_run install_name_tool -change /usr/local/Cellar/jpeg/8c/lib/libjpeg.8.dylib @executable_path/../Frameworks/libjpeg.8.dylib $bin
	try_run install_name_tool -change /usr/local/Cellar/jpeg/8b/lib/libjpeg.8.dylib @executable_path/../Frameworks/libjpeg.8.dylib $bin
	try_run install_name_tool -change /usr/local/Cellar/libtiff/3.9.5/lib/libtiff.3.dylib @executable_path/../Frameworks/libtiff.3.dylib $bin
	try_run install_name_tool -change /usr/local/Cellar/libtiff/3.9.5/lib/libtiffxx.3.dylib @executable_path/../Frameworks/libtiffxx.3.dylib $bin
	try_run install_name_tool -change /usr/local/Cellar/jasper/1.900.1/lib/libjasper.1.dylib @executable_path/../Frameworks/libjasper.1.dylib $bin
	
	if true; then
            for lib in *.dylib; do
		try_run install_name_tool -change @executable_path/$lib   @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change $lib                    @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change lib/$lib                @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change /opt/local/lib/$lib     @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change /usr/local/lib/$lib     @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change /usr/lib/$lib           @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change /usr/X11/lib/$lib       @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change ../../Bin/Release/$lib  @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change ../../../Bin/Release/$lib  @executable_path/../Frameworks/$lib $bin
		try_run install_name_tool -change /usr/lib/libcminpack.1.1.3.dylib @executable_path/../Frameworks/libcminpack.1.1.3.dylib $bin
            done
	fi
    done
fi

    # Copy some OpenNI plugins that also need to be in the MacOS dir


    cd ../MacOS

    rm -rf *.dylib
    for lib in ../Frameworks/lib{nim*,Xn*}.dylib; do ln -sf $lib; done

    rm -rf config plugins
    ln -s ../../../common/plugins .
    ln -s ../../../common/config .

    cd ..
    rm -rf PlugIns
    ln -s ../../common/Contents/PlugIns .

    cd ../..
done
