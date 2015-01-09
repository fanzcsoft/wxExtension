#!/bin/sh
################################################################################
# Name:      coverage.sh
# Purpose:   Coverage file (for wxExtension)
# Author:    Anton van Wezenbeek
# Copyright: (c) 2015 Anton van Wezenbeek
################################################################################

# Run this file in the build folder

BASEDIR=../extension
TESTDIR=./extension/src/CMakeFiles/wxex.dir

echo "-- make test coverage build --"
make

if [ $? != 0 ]; then
  echo "make failed"
  exit 1
fi

echo "-- lcov initializing --"
lcov --base-directory $BASEDIR --capture --initial --directory $TESTDIR --output-file app.base

./test-all.sh

if [ $? != 0 ]; then
  echo "test failed"
  exit 1
fi

echo "-- lcov collecting data --"
lcov --base-directory $BASEDIR --capture --directory $TESTDIR --output-file app.run
lcov --add-tracefile app.base --add-tracefile app.run --output-file app.total

# remove output for external and test libraries
lcov --remove app.total "/usr*" --output-file app.total
lcov --remove app.total "test/*" --output-file app.total
lcov --remove app.total "sample*" --output-file app.total
lcov --remove app.total "*wxExtension/sync*" --output-file app.total

# when manually updating coveralls
echo "-- sending to coveralls --"
coveralls-lcov --repo-token sOcBNDSlOjLSH3vs636zytnjuQtRWKLGP app.total

echo "-- genhtml building report --"
genhtml --no-branch-coverage --title "wxExtension Library" app.total

rm -f app.base app.run app.total
