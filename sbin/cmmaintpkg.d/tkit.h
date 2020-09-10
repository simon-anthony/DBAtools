#ifndef _HPUX_SOURCE
#pragma ident "$Header$"
#else
static char *svnid_tkit_h = "$Header$";
#endif
/* Copyright (C) 2008 Simon Anthony
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License or, (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program. If not see <http://www.gnu.org/licenses/>>
 */ 
#define CMDIR	"/etc/cmcluster"

#define MAX_NAME	40			/* max length of package variable (39) + \0 */

#define OC_DBG	"oc.debug"		/* name of debug file */
#define RAC_DBG	"rac.debug"

#define OC_CNF	"oc.conf"		/* name of configuration file */
#define RAC_CNF	"rac_dbi.conf"

#define ENT_PAT	"toolkit_*.sh"	/* entry script */
#define OC_ENT	"toolkit_oc.sh"
#define RAC_ENT	"toolkit_dbi.sh"

#define TKIT_OC		0
#define TKIT_RAC	1

#define SMAX		8			/* maximum length of oracle_sid */
#define MFLAG	"MAINTENANCE_FLAG"
#define OC_TKIT	"OC_TKIT_DIR"

#define TK_UNKNOWN		0x000
#define TK_ENABLED		0x001
#define TK_IMPLICIT		0x002
#define TK_DISABLED		0x004
#define TK_NOTIMPL		0x008

#define TK_ISENABLED(mode)	((mode)&TK_ENABLED)
#define TK_ISIMPLICIT(mode)	((mode)&TK_IMPLICIT)
#define TK_ISDISABLED(mode)	((mode)&TK_DISABLED)
#define TK_ISNOTIMPL(mode)	((mode)&TK_NOTIMPL)

#define NO			0
#define YES			1

struct tkit {
	int		type;
	char	pkgname[MAX_NAME];
	char	tkit_dir[PATH_MAX];	/* configuration directory of package */
	char	*conf;
	char	*entry_script;
	char	*debug;
	int		mflg;
};

#ifdef _TKIT
static struct tkit tkits[] = {
	{ TKIT_OC,  "", "", OC_CNF,  OC_ENT,  OC_DBG,  TK_UNKNOWN },
	{ TKIT_RAC, "", "", RAC_CNF, RAC_ENT, RAC_DBG, TK_UNKNOWN },
	0
};
#else
#ifdef __STDC__
extern struct tkit *tkit_stat(char *pkgname);
extern int tkit_getmmode(struct tkit *tk);
extern int tkit_setmmode(struct tkit *tk, int flag);
#else
extern struct tkit * tkit_stat();
extern int tkit_getmmode();
extern int tkit_setmmode();
#endif
#endif
