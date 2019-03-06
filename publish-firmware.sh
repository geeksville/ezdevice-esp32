set -e

if [ $# -lt 1 ]; then
  echo 1>&2 "$0: not enough arguments"
  exit 2
fi

BUCKETNAME=joyfirmware

if [ -z "$BUCKETNAME" ]; then
  echo "ERROR: Bucketname not set in environment"
  exit 1
fi

BOARDTYPE=$1 # R or M
FILENAME=firmware-$BOARDTYPE.bin
# aws s3 mb s3://$BUCKETNAME
aws s3 cp --acl public-read .pioenvs/featheresp32/firmware.bin s3://joyfirmware/$FILENAME
cp .pioenvs/featheresp32/firmware.bin releases/$FILENAME
echo Firmware published as https://$BUCKETNAME.s3.amazonaws.com/$FILENAME
