#!/usr/bin/ksh
# vim:syntax=sh:ts=4:sw=4:
################################################################################
# menu: options frmo ctx file
#
################################################################################
# Copyright (C) 2008 Simon Anthony
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License or, (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program. If not see <http://www.gnu.org/licenses/>>
#
menu() {
	typeset prefix="/opt/oracle"
	typeset ctxfile= s_inst_base= s_apps_version= \
		s_db_home_file= s_compatible= choice= default=
	typeset tmpfile=`$prefix/bin/mktemp -c`
	typeset s_systemname=$_sid

	while getopts s:h opt $* 2>&-
	do
		case $opt in
		s)	s_systemname="$OPTARG"
			sflg=y;;
		h)	errflg=y ;;
		\?)	errflg=y
		esac
	done
	shift $(( OPTIND - 1 ))

	[ $# -gt 0 ] && errflg=y

	[ $errflg ] && { print -u2 "usage: $prog [-s s_systemname] [run|patch]"; return 1; }

	if ctxfile=`getctx -as $s_system_name`
	then
		eval `ctxvar -ifv s_inst_base $ctxfile`
		eval `ctxvar -ifv s_apps_version $ctxfile`

		if [ -r $s_inst_base/EBSapps.env ]
		then
			echo "RUN $s_apps_version EBS Run environment" >> $tmpfile
			echo "PATCH $s_apps_version EBS Patch environment" >> $tmpfile
			default=RUN
		fi
	fi

	if ctxfile=`getctx -ds $s_system_name`
	then
		eval `ctxvar -ifv s_db_home_file $ctxfile`
		eval `ctxvar -ifv s_compatible $ctxfile`

		if [ -r $s_db_home_file ]
		then
			echo "DB $s_compatible Database environment" >> $tmpfile
			default=DB
		fi
	fi

	typeset -l choice=`ckitem -n ${default:+-d $default} -f $tmpfile`

	case $choice in
	run|patch)	echo . $s_inst_base/EBSapps.env $choice ;;
	db)			echo . $s_db_home_file
	esac
}
