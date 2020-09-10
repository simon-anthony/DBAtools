# $Header$

dist_man1_MANS = \
	adcfg.1 \
	adcpusr.1 \
	adctl.1 \
	adenv.1 \
	adpasswd.1 \
	adps.1 \
	adsessions.1 \
	adstatus.1 \
	adtns.1 \
	ckdba.1 \
	ckoper.1 \
	ckrac.1 \
	ckres.1 \
	ctx.1 \
	ctxvar.1 \
	dbh.1 \
	getctx.1 \
	getprb.1 \
	intro.1t \
	nbread.1 \
	pset.1 \
	rwid.1 \
	rwtop.1 \
	setapps.1 \
	setdb.1 \
	suboavars.1 \
	adbatch.1m \
	adstart.1m \
	adstop.1m \
	adsvcctl.1m \
	dbacmd.1m \
	dbshut.1m \
	dbstart.1m \
	dbvers.1m \
	lsnrstart.1m \
	lsnrstop.1m \
	oratab.1m  

if HPUX
dist_man1_MANS += cmmaintpkg.1m cmvgpkg.1m ptree.1 lsp.1 racp.1 ps.1
endif
if AIX
dist_man1_MANS += ptree.1 pgrep.1 dos2ux.1 ux2dos.1
endif

install-data-hook:
	@echo "g/BASEDIR/s;BASEDIR;$(prefix);g" > $(DESTDIR)/$(man1dir)/exscript ;\
	echo "g/SBINDIR/s;SBINDIR;$(sbindir);g" >> $(DESTDIR)/$(man1dir)/exscript ;\
	echo "g/BINDIR/s;BINDIR;$(bindir);g" >> $(DESTDIR)/$(man1dir)/exscript ;\
	echo "g/LIBDIR/s;LIBDIR;$(libdir);g" >> $(DESTDIR)/$(man1dir)/exscript ;\
	echo "g/DATADIR/s;DATADIR;$(datadir);g" >> $(DESTDIR)/$(man1dir)/exscript ;\
	echo "g/SYSCONFDIR/s;SYSCONFDIR;$(sysconfdir);g" >> $(DESTDIR)/$(man1dir)/exscript ;\
	echo "g/LOCALSTATEDIR/s;LOCALSTATEDIR;$(localstatedir);g" >> $(DESTDIR)/$(man1dir)/exscript ;\
	echo "g/PREFIX/s;PREFIX;$(prefix);g" >> $(DESTDIR)/$(man1dir)/exscript ;\
	echo "w!" >> $(DESTDIR)/$(man1dir)/exscript 
	@cd $(DESTDIR)/$(man1dir) ;\
	 for file in $(dist_man1_MANS) ;\
	 do \
	 	ex -s $$file < $(DESTDIR)/$(man1dir)/exscript ;\
	 done 
	@rm -f $(DESTDIR)/$(man1dir)/exscript

