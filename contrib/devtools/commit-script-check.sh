#!/bin/sh
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# This simple script checks for commits beginning with: scripted-diff:
# If found, looks for a script between the lines -BEGIN VERIFY SCRIPT- and
# -END VERIFY SCRIPT-. If no ending is found, it reads until the end of the
# commit message.

# The resulting script should exactly transform the previous commit into the current
# one. Any remaining diff signals an error.

if test "x$1" = "x"; then
    echo "Usage: $0 <commit>..."
    exit 1
fi

RET=0
PREV_BRANCH=`mynt-cli name-rev --name-only HEAD`
PREV_HEAD=`mynt-cli rev-parse HEAD`
for i in `mynt-cli rev-list --reverse $1`; do
    if mynt-cli rev-list -n 1 --pretty="%s" $i | grep -q "^scripted-diff:"; then
        mynt-cli checkout --quiet $i^ || exit
        SCRIPT="`mynt-cli rev-list --format=%b -n1 $i | sed '/^-BEGIN VERIFY SCRIPT-$/,/^-END VERIFY SCRIPT-$/{//!b};d'`"
        if test "x$SCRIPT" = "x"; then
            echo "Error: missing script for: $i"
            echo "Failed"
            RET=1
        else
            echo "Running script for: $i"
            echo "$SCRIPT"
            eval "$SCRIPT"
            mynt-cli --no-pager diff --exit-code $i && echo "OK" || (echo "Failed"; false) || RET=1
        fi
        mynt-cli reset --quiet --hard HEAD
     else
        if mynt-cli rev-list "--format=%b" -n1 $i | grep -q '^-\(BEGIN\|END\)[ a-zA-Z]*-$'; then
            echo "Error: script block marker but no scripted-diff in title"
            echo "Failed"
            RET=1
        fi
    fi
done
mynt-cli checkout --quiet $PREV_BRANCH 2>/dev/null || mynt-cli checkout --quiet $PREV_HEAD
exit $RET
