#ident $Header$

PATH=$HOME/bin:/opt/freeware/bin:/usr/bin:/opt/IBM/xlC/13.1.3/bin export PATH

[ -r ~/.rpmmacros ] ||  { echo "ERROR: cannot read ~/.rpmmacros" >&2; exit 1; }

topdir=`eval echo \`sed -n '
	/^%_topdir/ {
		s;%_topdir[     ]*;;
		s;%{getenv:HOME};$HOME; 
		p
	}' ~/.rpmmacros\``
#%_topdir %{getenv:HOME}/rpm
#%_buildrootdir %_topdir/BUILDROOT

[ "X$topdir" != "X" ] || { echo "ERROR: _topdir not set in .rpmmacros" >&2; exit 1; }

echo topdir is $topdir

for dir in BUILD BUILDROOT RPMS SOURCES SPECS SRPMS
do
	mkdir -p $topdir/$dir
done

autoreconf --install || exit $?
./configure \
	--with-oracle-home=/apps/DEV1/PRN/db/tech_st/12.1.0.2 \
	CC=xlc CFLAGS= LDFLAGS=-brtl 

for spec in data/*.spec
do
	spec=`basename $spec`

	make dist-gzip || exit $?  # make dist-gzip PACKAGE=${spec%%-[0-9]*} || exit $?

	mv ${spec%-[]0-9].spec}.tar.gz $topdir/SOURCES; cp -f data/$spec $topdir/SPECS

	rpmbuild -bb data/$spec || exit $?
done
