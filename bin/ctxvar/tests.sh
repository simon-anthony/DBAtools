#!/bin/sh -
#ident $Header$
# vim:ts=4:sw=4:syntax=sh:
unset CONTEXT_FILE
ctxfile=tests.d/myctx.xml
./ctxvar  -f s_isForms
./ctxvar -f s_isForms < $ctxfile
./ctxvar -f s_isForms $ctxfile
./ctxvar -v s_isForms -f $ctxfile
./ctxvar -v s_isForms $ctxfile
#./ctxvar s_isForms
echo Test enforcement of -f
./ctxvar s_isForms -f
./ctxvar s_isForms -f $ctxfile
./ctxvar s_isForms < $ctxfile
./ctxvar s_isForms $ctxfile
CONTEXT_FILE=$ctxfile ./ctxvar -f s_isForms
CONTEXT_FILE=$ctxfile ./ctxvar -fi s_isForms
#CONTEXT_FILE=$ctxfile ./ctxvar -i s_isForms
CONTEXT_FILE=$ctxfile ./ctxvar s_isForms
CONTEXT_FILE=$ctxfile ./ctxvar -f s_isForms
CONTEXT_FILE=nosuchfile ./ctxvar s_isForms
echo Test edit
cp $ctxfile $ctxfile.copy
./ctxvar -e s_isForms=YES $ctxfile.copy
./ctxvar -fv s_isForms $ctxfile.copy
./ctxvar -f s_isForms $ctxfile.copy
rm -f $ctxfile.copy
