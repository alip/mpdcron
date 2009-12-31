#!/bin/sh
# vim: set sw=4 et sts=4 tw=80 :

# Notes about valgrind & mpdcron:
# - Debugging stats module under valgrind fails when the module does a DNS
#   resolution.

abspath=$(readlink -f "$0")
absdir=$(dirname "$abspath")

export G_SLICE=always-malloc
exec valgrind \
    --leak-check=full \
    --track-origins=yes \
    --suppressions="$absdir"/valgrind.suppressions \
    "$@"
