#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
eval `grep -s '^ORA_CRS_HOME=' /etc/cmcluster/*/oc.conf` 

if [ "$ORA_CRS_HOME" ]
then
	PATH=$ORA_CRS_HOME/bin:$PATH
fi

crs_stat | awk -F= '
BEGIN {	
	printf("%-30s%-16s%-9s%-9s%-9s\n",
		"Name", "Type", "Target", "State", "Node")
	for (i=0; i<73; i++)	
		printf("-")
	printf("\n")
}
/^$/ {
	printf("\n"); next
}
$1 == "NAME" {
	printf("%-30s", $2); next
}
$1 == "TYPE" {
	printf("%-16s", $2); next
}
$1 == "TARGET" {
	printf("%-9s", $2); next
}
$1 == "STATE" {
	if ($2 ~ /^ONLINE/) {
		printf("%-9s", substr($2, 1, index($2, " ")))
		printf("%-9s", substr($2, index($2, "on ")+3))
	} else
		printf("%-9s", $2)
	next
}'
