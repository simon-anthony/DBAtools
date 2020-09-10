# $Header$

bindir = $(datadir)/lib/functions
SUFFIXES = .sh

.sh:
	@echo "g/BASEDIR/s;BASEDIR;$(prefix);g" > exscript ;\
	echo "g/SBINDIR/s;SBINDIR;$(sbindir);g" >> exscript ;\
	echo "g/BINDIR/s;BINDIR;$(bindir);g" >> exscript ;\
	echo "g/LIBDIR/s;LIBDIR;$(libdir);g" >> exscript ;\
	echo "g/DATADIR/s;DATADIR;$(datadir);g" >> exscript ;\
	echo "g/SYSCONFDIR/s;SYSCONFDIR;$(sysconfdir);g" >> exscript ;\
	echo "g/PREFIX/s;PREFIX;$(prefix);g" >> exscript ;\
	echo "g/prefix=/s;prefix=\"[^\"]*\";prefix=\"$(prefix)\";" >> exscript ;\
	echo "w! $@" >> exscript 
	@ex -s $< < exscript
	@rm -f exscript
	@chmod +x $@

bin_SCRIPTS = \
	appspass \
	getoavars \
	getsubopts \
	menu \
	msg \
	oiconnect \
	preserve \
	realpath \
	setapps \
	setdb \
	setoavars \
	sg \
	sqlprompt \
	suboavars \
	xmlvar
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = \
	appspass.sh \
	getoavars.sh \
	getsubopts.sh \
	menu.sh \
	msg.sh \
	oiconnect.sh \
	preserve.sh \
	realpath.sh \
	setapps.sh \
	setdb.sh \
	setoavars.sh \
	sg.sh \
	sqlprompt.sh \
	suboavars.sh \
	xmlvar.sh 
