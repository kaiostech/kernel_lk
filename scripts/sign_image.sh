#!/bin/bash

# This script calls gensecimage to sign the image with engineering key,
# and returns
SIGNING_PATH=$1
FILE_TO_SIGN=$2

# Make a backup of the original file
cp $FILE_TO_SIGN $FILE_TO_SIGN.unsigned

# Copy over to gensecimage to sign
mkdir $SIGNING_PATH/images
cp $FILE_TO_SIGN $SIGNING_PATH/images/
pushd $SIGNING_PATH
python gensecimage.py --stage=qpsa:sign --section=appsbl --config=resources/8974_LA_gensecimage.cfg
popd

# Copy the signed image over
cp $SIGNING_PATH/output/appsbl/emmc_appsboot.mbn $FILE_TO_SIGN
