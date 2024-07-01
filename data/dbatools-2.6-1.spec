# $Header: DBAtools/trunk/data/dbatools-2.6-1.spec 188 2017-11-16 16:16:24Z xanthos $
# vim:syntax=conf:filetype=spec:
%define _prefix /opt/oracle
%define _sysconfdir /etc/%{_prefix}

Prefix:			/opt/oracle

Name:			dbatools
Summary:		Tools for the Oracle DBA
Version:		2.6
Release:		1
Group:			Utilities/System
License:		GPL
Packager:		Simon Anthony
Source0:		%{name}-%{version}.tar.gz
BuildRoot:		%_topdir/BUILDROOT/%{name}-%{version}-%{release}-build

%description
DBAtools is a collection of programs designed to ease the administration of
Oracle databases.

%prep
%setup -q

%build
OBJECT_MODE=64 export OBJECT_MODE # for nm
%configure \
	--prefix=%{_prefix} \
	--bindir=%_bindir \
	--sbindir=%_sbindir \
	--datadir=%_datadir \
	--sysconfdir=%_sysconfdir \
	--libdir=%_libdir \
	--includedir=%_includedir \
	--localstatedir=/var/%{_prefix} \
	--mandir=%{_prefix}/share/man \
	--with-aix-soname=aix \
	--with-oracle-home=/opt/oracle/product/23ai/dbhomeFree \
	CC= CFLAGS= LDFLAGS=
make

%install
[ ${RPM_BUILD_ROOT} != "/" ] && rm -rf ${RPM_BUILD_ROOT}
make DESTDIR=${RPM_BUILD_ROOT} install

%clean
[ ${RPM_BUILD_ROOT} != "/" ] && rm -rf ${RPM_BUILD_ROOT}

%post
[ -f %{_bindir}/dos2ux ] && ln -f %{_bindir}/dos2ux %{_bindir}/ux2dos
ln -f %{_sbindir}/dbacmd  %{_sbindir}/dbvers
ln -f %{_sbindir}/dbacmd  %{_sbindir}/dbid
ln -f %{_sbindir}/dbstart %{_sbindir}/adstart
ln -f %{_sbindir}/dbstart %{_sbindir}/adstop
ln -f %{_sbindir}/dbstart %{_sbindir}/dbshut
ln -f %{_sbindir}/dbstart %{_sbindir}/lsnrstart
ln -f %{_sbindir}/dbstart %{_sbindir}/lsnrstop

ln -f %{_datadir}/lib/adstart.sh   %{_datadir}/lib/adstop.sh
ln -f %{_datadir}/lib/lsnrstart.sh %{_datadir}/lib/lsnrstop.sh

if [ ! -f %{_sysconfdir}/rwpat ]
then
	cp -pf %{_sysconfdir}/rwpat.example %{_sysconfdir}/rwpat
	chmod 664 %{_sysconfdir}/rwpat
else
	if egrep -q '{ORACLE_SID}|\${ctx}' %{_sysconfdir}/rwpat
	then
		cp -pf %{_sysconfdir}/rwpat.example %{_sysconfdir}/rwpat
		# old format - replace it
	fi
	chmod 664 %{_sysconfdir}/rwpat
	chgrp dba %{_sysconfdir}/rwpat 2>/dev/null
fi
for path in %{_sysconfdir}/skel/user.*
do
	 mv $path ${path%user.*}.${path#*.}
done
	

%preun
[ -f %{_bindir}/dos2ux ] && rm -f %{_bindir}/ux2dos
rm -f %{_sbindir}/dbvers
rm -f %{_sbindir}/dbid
rm -f %{_sbindir}/adstart
rm -f %{_sbindir}/adstop
rm -f %{_sbindir}/dbshut
rm -f %{_sbindir}/lsnrstart
rm -f %{_sbindir}/lsnrstop

rm -f %{_libdir}/adstop.sh
rm -f %{_libdir}/lsnrstop.sh

for path in %{_sysconfdir}/skel/.[a-z]*
do
	 mv $path ${path%%.*}user.${path#*.}
done

%files
%{_bindir}/*

%{_sbindir}/*

%{_includedir}/*

%{_libdir}/*.so*
%{_libdir}/*.a*

%attr(755,bin,dba) %{_datadir}/lib/*.sh
%dir %attr(775,bin,dba) %{_datadir}/lib/functions
%attr(755,bin,dba) %{_datadir}/lib/functions/*
%dir %attr(775,bin,dba) %{_datadir}/lib/sql
%attr(755,bin,dba) %{_datadir}/lib/sql/*
%dir %attr(775,bin,dba) %{_datadir}/nlsdata
%attr(755,bin,dba) %{_datadir}/nlsdata/*.nlb

%{_mandir}/man?/*

%attr(444,bin,dba) %{_sysconfdir}/rwpat.example
%dir %attr(775,bin,dba) %{_sysconfdir}/skel
%attr(444,bin,dba) %{_sysconfdir}/skel/*

