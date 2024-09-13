#!/bin/bash
#
# Outputs svn revision of directory $VERDIR into $TARGET if it changes $TARGET
# example output omitting enclosing quotes:
#
# '#define BL_VERS_REV "svn1676M"'
# '#define BL_VERS_MOD "vX.X.X.X"'
# '#define BL_VERS_OTH "build: username date hour"'

set -e

#VERDIR=$(dirname $(readlink -f $0))
#TARGET=$1

DATE_FORMAT="%b %d %Y %T"
#BL_VERS_MOD=$(grep BL_VERS_NUM $VERDIR/Makefile | cut -f2 -d=)

tmpout=bl_signed_gen.h
#cd $VERDIR

if (svn info . >& /dev/null)
then
    svnrev=svn$(svnversion -c | sed 's/.*://')
elif (git status -uno >& /dev/null)
then
    # append git info (sha1 and branch name)
    git_sha1=$(git rev-parse --short HEAD)
    svnrev=$git_sha1
else
    svnrev="Unknown Revision"
fi

date=$(LC_TIME=C date +"$DATE_FORMAT")

#BL_VERS_REV="$svnrev"
#      "lmac vX.X.X.X - build:"
#banner="${svnrev} by $(whoami) at $date"
banner="${svnrev}"

define() { echo "#define $1 \"$2\""; }
{
#	define "BL_VERS_REV"    "$BL_VERS_REV"
#	define "BL_VERS_MOD"    "$BL_VERS_MOD"
	define "CFG_REL_SIGNED" "$banner"
} > $tmpout

#cmp -s $TARGET $tmpout && rm -f $tmpout || mv $tmpout $TARGET

