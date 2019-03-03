set -e 

echo incrementing build number
awk -F " " '{printf("#define VERSION_NUM %d\n", $3+1)}' include/buildnum.h >/tmp/newbuild.h
mv /tmp/newbuild.h include/buildnum.h