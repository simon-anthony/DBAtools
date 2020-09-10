#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif
/* vim: syntax=c:sw=4:ts=4:
 *******************************************************************************
 * 
 *  Print out Apps 11i/12 service status from XML environment file. This implements
 *  the functionatilty formerly provided by ServiceControl.class
 *
 *******************************************************************************
 * Copyright (C) 2008 Simon Anthony
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
 
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#ifndef ORATYPES
#include <oratypes.h>
#endif
#ifndef ORAXML_ORACLE
#include <oraxml.h>
#endif

static int r12 = 0;
 
#define OA_GROUPS			"oa_service_group_list"
#define OA_GROUP			"oa_service_group"
#define OA_GROUP_STATUS		"oa_service_group_status"
#define OA_PROCESSES		(r12 ? "oa_service_list"   : "oa_processes")
#define OA_PROCESS			(r12 ? "oa_service"        : "oa_process")
#define OA_PROCESS_NAME		(r12 ? "oa_service_name"   : "oa_process_name")
#define OA_PROCESS_STATUS 	(r12 ? "oa_service_status" : "oa_process_status")

#define CTRL_SCRIPT			"ctrl_script"

static char *ora_nls10;

#define USAGE				"[-r 11|12] -e <envfile>"

static char *prog;

static void dump(xmlctx *ctx, xmlnode *node);
static void service_groups(xmlctx *ctx, xmlnode *node);
static void printchild(xmlnode *node, const char* child, const char *type, const char *svcstatus);
static void printservicegroup(xmlnode *node, const char* child);
static void dumpservice(xmlctx *ctx, xmlnode *node, const char *type, const char *svcstatus);

static xmlctx *ctx;

static int printed;
 
int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	extern	char *optarg;
	extern	int optind;
	uword	ecode;
	char	*file;
	int		rflg = 0, eflg = 0, errflg = 0;
	int		c;
 
	prog = basename(argv[0]);

	while ((c = getopt(argc, argv, "r:e:")) != -1)
		switch (c) {
		case 'r':		/* R12? */
			rflg++;
			r12 = (strcmp(optarg, "12") == 0) ;
			break;
		case 'e':		/* envfile */
			eflg++;
			file = optarg;
			break;
		case '?':
			errflg++;
		}

	if (!eflg)
		errflg++;

	if (errflg) {
		fprintf(stderr, "%s: %s\n", prog, USAGE);
		exit(2);
	}

	if (access(file, R_OK) != 0) {
		fprintf(stderr, "%s: cannot open '%s'\n", prog, file);
		exit(1);
	}

	if ((ora_nls10 = getenv("ORA_NLS10")) == NULL) {
		if ((ora_nls10 = (char *)malloc(
				strlen(ORA_NLS10) + strlen("ORA_NLS10=") + 1)) == NULL) {
			perror("malloc");
			exit(1);
		}
		snprintf(ora_nls10, PATH_MAX, "ORA_NLS10=%s", ORA_NLS10);
		if (putenv(ora_nls10) != 0) {
			perror("putenv");
			exit(1);
		}
	}
	
	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}

    if (!(ctx = xmlinit(&ecode, (const oratext *) 0,
                        (void (*)(void *, const oratext *, uword)) 0, 
                        (void *) 0, (const xmlsaxcb *) 0, (void *) 0,
						(const xmlmemcb *) 0, (void *) 0, 
                        (const oratext *) 0))) {
		fprintf(stderr, "%s: failed to initialze XML parser [%u]\n",
			prog, (unsigned) ecode);
		exit(1);
	}

    if (ecode = xmlparse(ctx, (oratext *)file, (oratext *) 0,
			XML_FLAG_VALIDATE | XML_FLAG_DISCARD_WHITESPACE)) {
		fprintf(stderr, "%s: parse failed [%u]\n", prog, (unsigned) ecode);
		exit(1);
	}
	if (r12)
		printf("[Service Control Execution Report]\nThe report format is:\n  <Service Group>            <Service>                                  <Script>         <Status>\n\n");
	else
		printf("[Service Control Execution Report]\nThe report format is:\n  <Service>                                             <Script>        <Status>\n\n");

	if (r12)
		service_groups(ctx, getDocumentElement(ctx));
	else
		dump(ctx, getDocumentElement(ctx));

	xmlterm(ctx);

	printf("\n\nServiceControl is exiting with status 0\n\n");

	exit(0);
}

static void
#ifdef __STDC__
dump(xmlctx *ctx, xmlnode *node)
#else
dump(ctx, node)
 xmlctx *ctx;
 xmlnode *node;
#endif
{
    const oratext *name;
    void    *nodes;
    uword    i, n_nodes;
	char	*v;
 
    name = getNodeName(node);

	if (!strcmp((char *) name, OA_PROCESSES))
		printchild(node, OA_PROCESS, NULL, NULL);

    if (hasChildNodes(node)) {
        nodes = getChildNodes(node);
        n_nodes = numChildNodes(nodes);
        for (i = 0; i < n_nodes; i++)
            dump(ctx, getChildNode(nodes, i));
    }
}

static void
#ifdef __STDC__
dumpservice(xmlctx *ctx, xmlnode *node, const char *type, const char *svcstatus)
#else
service(ctx, node)
 xmlctx *ctx;
 xmlnode *node;
 oratext *types;
#endif
{
    const oratext *name, *attrvalue;
    void    *nodes;
    uword    i, n_nodes;
	char	*v;
 
    name = getNodeName(node);

	if (!strcmp((char *) name, OA_PROCESSES))
		printchild(node, OA_PROCESS, type, svcstatus);

    if (hasChildNodes(node)) {
        nodes = getChildNodes(node);
        n_nodes = numChildNodes(nodes);
        for (i = 0; i < n_nodes; i++)
            dumpservice(ctx, getChildNode(nodes, i), type, svcstatus);
    }
}

static void
#ifdef __STDC__
service_groups(xmlctx *ctx, xmlnode *node)
#else
service_groups(ctx, node)
 xmlctx *ctx;
 xmlnode *node;
#endif
{
    const oratext *name, *attrvalue;
    void    *nodes;
    uword    i, n_nodes;
	char	*v;
 
    name = getNodeName(node);

	if (!strcmp((char *) name, OA_GROUPS))
		printservicegroup(node, OA_GROUP);

    if (hasChildNodes(node)) {
        nodes = getChildNodes(node);
        n_nodes = numChildNodes(nodes);
        for (i = 0; i < n_nodes; i++)
            service_groups(ctx, getChildNode(nodes, i));
    }
}

static void
#ifdef __STDC__
printchild(xmlnode *node, const char *child, const char *type, const char *svcstatus)
#else
printchild(node, child, type, svcstatus)
 xmlnode *node;
 char *child;
 char *type;
 char *svcstatus;
#endif
{
	const oratext *value, *name = getNodeName(node);
    void	*nodes;
    uword	i, n_nodes;
	char	*process = NULL, *script = NULL, *status = NULL;

	if (hasChildNodes(node)) {
		nodes = getChildNodes(node);
		n_nodes = numChildNodes(nodes);

		for (i = 0; i < n_nodes; i++) {
			if (name = getNodeName(getChildNode(nodes, i))) {
				if (!strcmp((char *)name, OA_PROCESS_NAME) ||
					!strcmp((char *)name, OA_PROCESS_STATUS) ||
					!strcmp((char *)name, CTRL_SCRIPT)) {
					if (value = getNodeValue(getFirstChild(getChildNode(nodes, i)))) {
						if (!strcmp((char *)name, OA_PROCESS_NAME))
							process = strdup((char *)value);
						else if (!strcmp((char *)name, OA_PROCESS_STATUS))
							status = strdup((char *)value);
						else if (!strcmp((char *)name, CTRL_SCRIPT))
							script = strdup((char *)value);
					}
				}
			}
			if (!strcmp(child, OA_PROCESS))
				printchild(getChildNode(nodes, i), "", type, svcstatus);
		}
		if (process && script && status) {
			if (type) {
				const oratext  *attrvalue;
				attrvalue = getAttribute(node, (const oratext *)"type");
				if (strcmp((char *)attrvalue, type) != 0)
					goto skip;

				printf(" %-42s %-16s", process, basename((char *)script));

				if (strcmp(svcstatus, "Disabled") == 0)
					printf(" %s\n", svcstatus);
				else
					printf(" %s\n", !strcmp(status, "disabled") ? "Disabled" : "");

				printed = 1;

			} else  
				printf("  %-53s %-15s %s\n",
					process, basename((char *)script),
					!strcmp(status, "disabled") ? "Disabled" : "");
			skip:
				free(process); free(script); free(status);
		}
	}
}

static void
#ifdef __STDC__
printservicegroup(xmlnode *node, const char *child)
#else
printservicegroup(node, child)
 xmlnode *node;
 char *child;
#endif
{
	const oratext *value, *name = getNodeName(node), *title, *attrvalue;
    void	*nodes;
    uword	i, n_nodes;
	char	*status = NULL;
	xmlnodes *list;

	if (hasChildNodes(node)) {
		nodes = getChildNodes(node);
		n_nodes = numChildNodes(nodes);

		if (strcmp((char *)child, OA_GROUP)) {
			title = getAttribute(node, (const oratext *)"title");
			printf("  %-87s", (char *)title);
		}

		for (i = 0; i < n_nodes; i++) {
			if (name = getNodeName(getChildNode(nodes, i))) {
				if (!strcmp((char *)name, OA_GROUP_STATUS)) {
					if (value = getNodeValue(getFirstChild(getChildNode(nodes, i)))) {
						if (!strcmp((char *)name, OA_GROUP_STATUS))
							status = strdup((char *)value);
					}
				}
			}
			if (!strcmp(child, OA_GROUP))
				printservicegroup(getChildNode(nodes, i), "");
		}
		if (status) {
			status[0] = toupper(status[0]);
			printf("%s\n",  status);
			//free(status);
		}
		if (strcmp((char *)child, OA_GROUP)) {
			char *service;

			attrvalue = getAttribute(node, (const oratext *)"services");

			while ((service = strtok((char *)attrvalue, " ,\t")) != NULL) {
				attrvalue = NULL;
				
				printf("  %-26s", title);
				printed = 0;
				dumpservice(ctx, getDocumentElement(ctx), service, (char *)status);
			
				if (!printed)
					printf(" %-42s %-16s %s\n", service, "-", "Undefined");
				//	printf(" %-42s %-16s\n", service, "undefined");
			}
		}
		if (status)
			free(status);
	}
}
