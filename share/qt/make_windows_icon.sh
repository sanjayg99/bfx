#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/bfx.ico

convert ../../src/qt/res/icons/bfx-16.png ../../src/qt/res/icons/bfx-32.png ../../src/qt/res/icons/bfx-48.png ${ICON_DST}
