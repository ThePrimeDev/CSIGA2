#!/bin/sh
DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$DIR/ld-linux-x86-64.so.2" --library-path "$DIR" "$DIR/CSIGA2" "$@"
