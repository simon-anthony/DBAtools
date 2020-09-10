#ifndef _HPUX_SOURCE
#pragma ident "$Header$"
#else
static char *svnid = "$Header$";
#endif
/* A pmap utility for HP-UX
 * It uses HP-UX's pstat interface to obtain the information.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/pstat.h>
#include <errno.h>
#include <fenv.h>
#include <unistd.h>
#include <libgen.h>
#define USAGE "[ [-x] | [-o <options...>] | [-g] ] <pid>"

#define PAGE_SIZE	4096
#define K	1024
#define M	1048576
#define G	1073741824

#define P_HPUX		0x1
#define P_SYSV4		0x2
#define P_GLANCE	0x4

#ifdef __STDC__
static void  total(int, _T_LONG_T, _T_LONG_T);
static void  xtotal(_T_LONG_T, _T_LONG_T, _T_LONG_T, _T_LONG_T);
static void  print_total(void);
static void  print_xtotal(void);
static const char *permission_string(long);
static char *flags(long);
//static const char *flags_string(long);
static void print_type(int, int);
static char *canonicalize2(register _T_LONG_T); 
static char *canonicalize3(register _T_LONG_T); 
//static void  get_shmid(int);
static void  pmap_type(int, _T_LONG_T, _T_LONG_T);
#else
static void  total();
static void  xtotal();
static void  print_total();
static void  print_xtotal();
static char *permission_string();
static char *flags();
//static char *flags_string();
static void print_type();
static char *canonicalize2(); 
static char *canonicalize3(); 
//static void  get_shmid();
static void  pmap_type();
#endif 

static char *prog;
static int sysv4 = 0;
static int oldsysv4 = 0;
static long 	sc_page_size;
static struct pst_vm_status pst;
static struct pst_fileinfo2 psf;
static struct pst_filedetails psfd;

static int xflg = 0, oflg = 0, gflg = 0, errflg = 0;

static char *myopts[] = {
#define MLCK		0
#define MLCK_FMT	"%7uK"
					"mlck",
#define NAME		1
#define NAME_FMT	"%s"
					"name",
#define OFFSET		2
#define OFFSET_FMT	"%08X"
					"offset",
#define PERM		3
#define PERM_FMT	"%16s"
					"perm",
#define RSZ			4
#define RSZ_FMT		"%7uK"
					"rsz",
#define SPACE		5
#define SPACE_FMT	"%7uK"
					"space",
#define SWAP		6
#define SWAP_FMT	"%uK"
					"swap",
#define SZHNT		7
#define SZHNT_FMT	"%uK"
					"szhnt",
#define TYPE		8
#define TYPE_FMT	"%8s"
					"type",
#define VSZ			9
#define VSZ_FMT		"%7uK"
					"vsz",
					NULL
};

static int pmapoptions[25];

static _T_LONG_T text_rss = 0, data_rss = 0, stack_rss = 0, shmem_rss = 0,
		other_rss = 0;
static _T_LONG_T text_vss = 0, data_vss = 0, stack_vss = 0, shmem_vss = 0,
		other_vss = 0;

/* For SysV4 -x total */
static _T_LONG_T tot_vss = 0, tot_rss = 0, tot_anon = 0, tot_locked = 0;

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	unsigned long pid;
	char	buf[BUFSIZ];
	int		idx, nopts = 0;
	char	*options, *value;
	extern	char *optarg;
	extern	int optind, opterr;
	int		c;

	prog = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "gxo:")) != -1)
		switch (c) {
		case 'o':		/* address range */
			if (xflg||gflg)
				errflg++;
			oflg++;
			options = optarg;
			while (*options != '\0') {
				switch(c = getsubopt(&options, myopts, &value)) {
				case MLCK:
				case NAME:
				case OFFSET:
				case PERM:
				case RSZ:
				case SPACE:
				case SWAP:
				case SZHNT:
				case TYPE:
				case VSZ:
					pmapoptions[nopts++] = c;
					break;
				default:
					errflg++;
					break;
				}
			}
			break;
		case 'x':		/* extended info */
			if (oflg||gflg)
				errflg++;
			xflg++;
			sysv4++;
			break;
		case 'g':		/* Glance format */
			if (oflg||xflg)
				errflg++;
			gflg++;
			break;
		case '?':
			errflg++;
			break;
		}

	if(argc-optind != 1) 
		errflg++;

	if (errflg) {
		fprintf(stderr, "%s: %s\n", prog, USAGE);
		exit(2);
	}
	pid = atol(argv[optind]);
	fesetround(FE_UPWARD);

	if ((sc_page_size = sysconf(_SC_PAGE_SIZE)) == -1) {
		perror("sysconf");
		exit(1);
	}
	
	if (sysv4) {
		if (xflg)
			printf("Address    Kbytes     RSS    Anon  Locked PgSz Mode    Mapped File\n");
			//printf("Address    Kbytes Mode    Mapped File\n");
	} else if (gflg)
		printf("Type          RefCt   RSS    VSS   Locked  File Name\n");

	idx = 0;
	for(;;) {
		/* we can only grab one region at a time */

		int n = pstat_getprocvm(&pst, sizeof(pst), pid, idx);

		if (n == 0)
			break; /* end */
		if (n == -1) {
			char msg[256];
			sprintf(msg, "pmap: Could not get VM info for process %ld", pid);
			perror(msg);
			return 1;
		}
		
		if (xflg) {
			char *fmt = "%08X  %7u %7u %7s %7s %3dK %-7s ";
			int anon = 0;

			if (pst.pst_type & PS_MMF) {
				char pathname[1024];
				if (pstat_getpathname(pathname, sizeof(pathname), &pst.pst_fid) < 0)
					anon++;
			}
			printf(fmt,
					(unsigned long)pst.pst_vaddr,
					(unsigned)(pst.pst_length*PAGE_SIZE/1024),
					(unsigned)(pst.pst_phys_pages*PAGE_SIZE/1024),
					anon > 0
						? ltoa((unsigned)(pst.pst_length*PAGE_SIZE/1024))
						: "-",
					pst.pst_lockmem > 0
						? ltoa((unsigned)(pst.pst_lockmem*PAGE_SIZE/1024))
						: "-",
					sc_page_size/K,
					permission_string(pst.pst_permission)
			);
			print_type((int)pst.pst_type, P_SYSV4);
			//if (pst.pst_type == PS_SHARED_MEMORY) {
			//printf("hello %ld\n", pst.pst_id.psf_fsid.psfs_id);
			//printf("hello %d\n", pst.pst_lo_nodeid);
			//get_shmid(pst.pst_id.psf_fileid);
			//}
			xtotal(pst.pst_length*PAGE_SIZE/1024,
					pst.pst_phys_pages*PAGE_SIZE/1024,
					anon > 0
						? pst.pst_length*PAGE_SIZE/1024
						: 0,
					pst.pst_lockmem > 0
						? pst.pst_lockmem*PAGE_SIZE/1024
						: 0);

		} else if (sysv4) {
			char *fmt = "%08X %7uK %-7s";

			if (oldsysv4)
				fmt = "%08X %7uK %-18s";
			printf(fmt,
				   (unsigned long)pst.pst_vaddr,
				   (unsigned)((pst.pst_length*PAGE_SIZE)/1024),
				   permission_string(pst.pst_permission)
			);
			print_type((int)pst.pst_type, P_SYSV4);
		} else if (gflg) {
			total((int)pst.pst_type, pst.pst_phys_pages, pst.pst_length);

			print_type((int)pst.pst_type, P_GLANCE);

			printf("/%-6s %4lld %6s %6s %8s ",
				flags(pst.pst_flags),
				pst.pst_refcnt,
				canonicalize2(pst.pst_phys_pages*PAGE_SIZE),
				canonicalize2(pst.pst_length*PAGE_SIZE),
				canonicalize2(pst.pst_lockmem*PAGE_SIZE)
			);
			print_type((int)pst.pst_type, 0);
		} else { /* default HP-UX 11.3: offset, rsz, type, perm, name */
			if (!oflg) {
				printf("%08X %7uK %6s %-16s ",
				   (unsigned long)pst.pst_vaddr,
				   (unsigned)((pst.pst_phys_pages*PAGE_SIZE)/1024),
					flags(pst.pst_flags),
					permission_string(pst.pst_permission)
				);
				print_type((int)pst.pst_type, 0);
			} else {
				int i, c;
				for (i = 0; i < nopts; i++) {
					if (i > 0) putchar(' ');
					switch(pmapoptions[i]) {
					case MLCK:
						printf(MLCK_FMT,
							(unsigned)pst.pst_lockmem*PAGE_SIZE/K); // changed
						break;
					case NAME:
						print_type((int)pst.pst_type, P_HPUX);
						break;
					case OFFSET:
						printf(OFFSET_FMT,
							(unsigned long)pst.pst_vaddr);
						break;
					case PERM:
						printf(PERM_FMT,
							permission_string(pst.pst_permission));
						break;
					case RSZ:
						printf(RSZ_FMT,
						   (unsigned)(pst.pst_phys_pages*PAGE_SIZE)/K);
						break;
					case SPACE:
					case SWAP:
					case SZHNT:
					case TYPE:
						//print_type((int)pst.pst_type, P_HPUX);
						pmap_type((int)pst.pst_type,
							pst.pst_flags, pst.pst_refcnt);
						break;
					case VSZ:
						printf(VSZ_FMT,
						   (unsigned)(pst.pst_length*PAGE_SIZE)/K);
						break;
					default:
						errflg++;
						break;
					}
				}
			}
			//if (pstat_getfile2(&psf, sizeof(psf), 0, &pst.pst_fid, pid) < 0) {
			//	perror("pstat_getfile2");
				//exit(1);
			//}
			//if (pstat_getfiledetails(&psfd, sizeof(psfd), &pst.pst_fid) < 0) {
			//	perror("pstat_getfiledetails");
			//}
		}
		printf("\n");
		
		idx++;
	}
	if (gflg)
		print_total();
	if (xflg)
		print_xtotal();
	
	
	return 0;
}
#ifdef __STDC__
static const char *permission_string(long p)
#else
static char *permission_string(p)
 long p;
#endif
{
	switch(p) {
		case 0:
			return "";
		case PS_PROT_READ:
			if (sysv4 & !oldsysv4)
				return "r----";
			else
				return "read";
		case PS_PROT_WRITE:
			if (sysv4 & !oldsysv4)
				return "-w---";
			else
				return "write";
		case PS_PROT_WRITE|PS_PROT_READ:
			if (sysv4 & !oldsysv4)
				return "rw---";
			else
				return "read/write";
		case PS_PROT_EXECUTE:
			if (sysv4 & !oldsysv4)
				return "--x--";
			else
				return "exec";
		case PS_PROT_EXECUTE|PS_PROT_READ:
			if (sysv4 & !oldsysv4)
				return "r-x--";
			else
				return "read/exec";
		case PS_PROT_EXECUTE|PS_PROT_WRITE:
			if (sysv4 & !oldsysv4)
				return "-wx--";
			else
				return "write/exec";
		case PS_PROT_EXECUTE|PS_PROT_WRITE|PS_PROT_READ:
			if (sysv4 & !oldsysv4)
				return "rwx--";
			else
				return "read/write/exec";
		default:
			return "?";
	}
}


/*
#ifdef __STDC__
static const char *flags_string(long f)
#else
static char *flags_string(f)
 long f;
#endif
{
	static char buf[128];
	buf[0]='\0';
	if(f&PS_MEMORY_LOCKED)  { if(buf[0]) strcat(buf," "); strcat(buf,"lock"); }
	if(f&PS_EXECUTABLE)     { if(buf[0]) strcat(buf," "); strcat(buf,"exec"); }
	if(f&PS_SHARED)         { if(buf[0]) strcat(buf," "); strcat(buf,"shrd"); }
	if(f&PS_SHARED_LIBRARY) { if(buf[0]) strcat(buf," "); strcat(buf,"shlib"); }
	return buf;
}
*/

static char *
#ifdef __STDC__
flags(long f)
#else
flags(f)
 long f;
#endif
{
	static char buf[128];
	buf[0]='\0';
	if (f&PS_SHARED) { if(buf[0]) strcat(buf," "); strcat(buf,"Shared"); }
	else { if(buf[0]) strcat(buf," "); strcat(buf,"Priv"); }
	return (char *)buf;
}

static void
#ifdef __STDC__
print_type(int type, int flg)
#else
print_type(type, flg)
 int type;
 int flg;
#endif
{
	char	buf[BUFSIZ];

	switch(type) {
	case PS_NOTUSED:
		printf("<notused>");
		break; /*probably never happens*/
	case PS_USER_AREA:
	/*	The uarea is implemented as a kernel memory region, and unlike
		the other regions mapped in the parent process's pregion chain,
		access to the uarea is private to a single kthread. If there are
		multiple siblings, then there is an equal number of uarea
		pregions linked to the process's vas. An interesting note is
		that the uarea is mapped into a process's vas but never directly
		accessed by a user thread. The kernel has exclusive usage rights
		and stores the threads register image, known as a process
		control block (PCB) prior to a context switch-out. It then loads
		the next thread on the run queue's PCB from its uarea prior to
		switching it in. */

		if (flg & P_HPUX)
			printf("uarea");
		else if (flg & P_SYSV4)
			printf(" [ uarea ]");
		else
			(flg & P_GLANCE) ? printf("%-6s", "UAREA") : printf("<uarea>");
		break;
	case PS_TEXT:
		if (flg & P_HPUX)
			printf("text");
		else if (flg & P_SYSV4)
			printf(" [ text ]");
		else
			(flg & P_GLANCE) ? printf("%-6s", "TEXT") : printf("<text>");
		break;
	case PS_DATA:
		if (flg & P_HPUX)
			printf("data");
		else if (flg & P_SYSV4)
			printf(" [ data ]");
		else
			(flg & P_GLANCE) ? printf("%-6s", "DATA") : printf("<data>");
		break;
	case PS_STACK:
		if (flg & P_HPUX)
			printf("stack");
		else if (flg & P_SYSV4)
			printf(" [ stack ]");
		else
			(flg & P_GLANCE) ? printf("%-6s", "STACK") : printf("<stack>");
		break;
	case PS_SHARED_MEMORY:
		if (flg & P_HPUX)
			printf("shared_memory");
		else if (flg & P_SYSV4)
			printf(" [ shmid ]");
		else
			(flg & P_GLANCE) ? printf("%-6s", "SHMEM") : printf("<shmem>");
		break;
	case PS_NULLDEREF:
		if (flg & P_HPUX)
			printf("nulldref");
		else if (flg & P_SYSV4)
			printf(" [ null ]");
		else
			(flg & P_GLANCE) ? printf("%-6s", "NULLDR") : printf("<nulldref>");
		break; /* special page mapped at addres 0 */
	case PS_IO:
		if (flg & P_HPUX)
			printf("io");
		else if (flg & P_SYSV4)
			printf("[ io ]");
		else
			(flg & P_GLANCE) ? printf("%-6s", "IO") : printf("<io>");
		break;
	case PS_MMF: 
		/* a memory-mapped file. grab the filename from the DNLC cache */
		if ((flg & P_GLANCE))
			printf("%-6s", "MEMMAP");
		else {
			char pathname[1024];
			if(pstat_getpathname(pathname, sizeof(pathname), &pst.pst_fid) > 0) {
				printf("%s", pathname);
			} else {
				/* Info not available. This can happen if
				 * the file has been deleted, if the mmap()
				 * was anonymous, or if the process/file
				 * belongs to another user*/
				if (flg & P_HPUX) {
					if (pstat_getfiledetails(&psfd, sizeof(psfd), &pst.pst_fid) < 0)
						printf("anonymous");
					else
						printf("%ld, %ld", (int)psfd.psfd_ino,  (int)psfd.psfd_dev); // changed
				} else if (flg & P_SYSV4)
					printf(" [ anon ]");
				else
					printf("<mmap>");
				}
		}
		break;
	case PS_GRAPHICS:
		if (flg & P_HPUX)
			printf("graphics");
		else if (flg & P_SYSV4)
			printf(" [ graphics ]");
		else
			printf("<graphics>");
		break;
	case PS_GRAPHICS_DMA:
		if (flg & P_HPUX)
			printf("dma_graphics");
		else if (flg & P_SYSV4)
			printf(" [ dma-graphics ]");
		else
			printf("<dma-graphics>");
		break;
	default: 
		printf("<UNKNOWN>");
		break;
	}
}

static char *
#ifdef __STDC__
canonicalize2(register _T_LONG_T n)
#else
canonicalize2(n) 
 register _T_LONG_T n;
#endif
{
	static	char s[10];
	char	*ind = "kb";
	int		div  = K;
	char	*fmt = "%.0f%s";


	if ((n / G) > 10000) {
		ind = "gb"; div = G; fmt = "%.2f%s";
		goto out;
	}
	if ((n / M) >= 1000) {
		ind = "gb"; div = G; fmt = "%.2f%s";
		goto out;
	}
	if ((n / M) >= 1) {
		ind = "mb"; div = M; fmt = "%.1f%s";
		goto out;
	}
	/*
	if ((n / K) >= 1000) {
		ind = "mb"; div = M; fmt = "%.1f%s";
		goto out;
	}
	*/
	if ((n / K) > 1) {
		ind = "kb"; div = K;
		goto out;
	}
out:
	snprintf(s, 10, fmt, ((float)n / div), ind);

	return strdup(s);
}
static char *
#ifdef __STDC__
canonicalize3(register _T_LONG_T n)
#else
canonicalize3(n) 
 register _T_LONG_T n;
#endif
{
	static	char s[10];
	static	char t[10];
	char	*ind = "kb";
	int		div  = K;
	char	*fmt = "%.0f%s";


	if ((n / G) > 10000) {
		ind = "gb"; div = G; fmt = "%.1f%s";
		goto out;
	}
	if ((n / M) >= 1000) {
		ind = "gb"; div = G; fmt = "%.1f%s";
		goto out;
	}
	if ((n / M) >= 10) {
		ind = "mb"; div = M; fmt = "%.0f%s";
		goto out;
	}
	if ((n / M) >= 1) {
		ind = "mb"; div = M; fmt = "%.1f%s";
		goto out;
	}
	if ((n / K) > 1) {
		ind = "kb"; div = K;
		goto out;
	}
out:
	snprintf(s, 10, fmt, ((float)n / div), ind);

	return strdup(s);
}
static void
#ifdef __STDC__
xtotal(_T_LONG_T vss, _T_LONG_T rss, _T_LONG_T anon, _T_LONG_T locked)
#else
xtotal(vss, rss, anon, locked)
 _T_LONG_T vss;
 _T_LONG_T rss;
 _T_LONG_T anon;
 _T_LONG_T locked;
#endif
{
	tot_vss += vss;
	tot_rss += rss;
	tot_anon += anon;
	tot_locked += locked;
}

static void
#ifdef __STDC__
total(int type, _T_LONG_T rss, _T_LONG_T vss)
#else
total(type, rss, vss)
 int type;
 _T_LONG_T rss;
 _T_LONG_T vss;
#endif
{
	char	buf[BUFSIZ];

	switch(type) {
	case PS_NOTUSED:
		break; /*probably never happens*/
	case PS_USER_AREA:
		other_rss += rss; other_vss += vss;
		break;
	case PS_TEXT:
		text_rss += rss; text_vss += vss;
		break;
	case PS_DATA:
		data_rss += rss; data_vss += vss;
		break;
	case PS_STACK:
		stack_rss += rss; stack_vss += vss;
		break;
	case PS_SHARED_MEMORY:
		shmem_rss += rss; shmem_vss += vss;
		break;
	case PS_NULLDEREF:
		other_rss += rss; other_vss += vss;
		break; /*special page mapped at addres 0*/
	case PS_IO:
		other_rss += rss; other_vss += vss;
		break;
	case PS_MMF: 
		other_rss += rss; other_vss += vss;
		break;
	case PS_GRAPHICS:
		other_rss += rss; other_vss += vss;
		break;
	case PS_GRAPHICS_DMA:
		other_rss += rss; other_vss += vss;
		break;
	default: 
		printf("<UNKNOWN>");
		break;
	}
}

static void
#ifdef __STDC__
print_total(void)
#else
print_total()
#endif
{
	printf("\n");
	printf("Text  RSS/VSS:%5s/%5s  Data  RSS/VSS:%5s/%5s  Stack RSS/VSS:%5s/%5s\n",
		canonicalize3(text_rss*PAGE_SIZE),
		canonicalize3(text_vss*PAGE_SIZE),
		canonicalize3(data_rss*PAGE_SIZE),
		canonicalize3(data_vss*PAGE_SIZE),
		canonicalize3(stack_rss*PAGE_SIZE),
		canonicalize3(stack_vss*PAGE_SIZE));

	printf("Shmem RSS/VSS:%5s/%5s  Other RSS/VSS:%5s/%5s\n",
		canonicalize3(shmem_rss*PAGE_SIZE),
		canonicalize3(shmem_vss*PAGE_SIZE),
		canonicalize3(other_rss*PAGE_SIZE),
		canonicalize3(other_vss*PAGE_SIZE));
}

static void
#ifdef __STDC__
print_xtotal(void)
#else
print_xtotal()
#endif
{
	printf("          ------- ------- ------- -------\n");
	printf("total Kb  %7u %7u %7u %7u\n",
		(unsigned)tot_vss,
		(unsigned)tot_rss,
		(unsigned)tot_anon,
		(unsigned)tot_locked);
}

/*
static void
#ifdef __STDC__
get_shmid(int id)
#else
get_shmid(id)
 int id;
#endif
{
	struct pst_shminfo *pss;

	if (pstat_getshm(pss, sizeof(struct pst_shminfo), 0, id) != -1) {

		(void)printf("%12s %15s %15s %15s %15s %15s\n",
			"owner", "key", "size", "seq", "cpid", "shmid");

		(void)printf("%12d %#15x %15d %15d %15d %15d\n",
		(int)pss[0].psh_uid, // changed all
		(int)pss[0].psh_key,
		(int)pss[0].psh_segsz,
		(int)pss[0].psh_seq,
		(int)pss[0].psh_cpid,
		(int)pss[0].psh_idx);
	}
	else
		perror("pstat_getshm");
}
*/

static void
#ifdef __STDC__
pmap_type(int type, _T_LONG_T flags, _T_LONG_T refcnt)
#else
pmap_type(type, flags, refcnt)
 int type;
 _T_LONG_T flags;
 _T_LONG_T refcnt;
#endif
{
	char	buf[BUFSIZ];

	switch(type) {
	case PS_NOTUSED:
		break; /*probably never happens*/
	case PS_DATA:
	case PS_SHARED_MEMORY:
	case PS_NULLDEREF:
		if (flags&PS_SHARED)
			printf("SD(%lld)", refcnt); 
		else
			printf("PD"); 
		break; /*special page mapped at addres 0*/
	case PS_USER_AREA:
	case PS_STACK:
	case PS_TEXT:
	case PS_MMF: 
	case PS_IO:
	case PS_GRAPHICS:
	case PS_GRAPHICS_DMA:
		if (flags&PS_SHARED)
			printf("SC(%lld)", refcnt); 
		else
			printf("PC"); 
		break;
	default: 
		printf("<UNKNOWN>");
		break;
	}
}
