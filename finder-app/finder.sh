#!/bin/sh

if [ $# -lt 2 ]
then
    echo Must provide 2 arguments, only $# given!
    exit 1
fi

if [ ! -d $1 ]
then
    echo The second argument must be a directory and it must exist!
    exit 1
fi

numFiles=$( ls $1 | wc -l )
numLines=$( grep -r $2 $1 | wc -l )

# More than one way to skin a cat...
#numFiles=$( find $1 -true -type f | wc -l )
#numLines=$( find $1 -true -type f | xargs cat | grep $2 | wc -l )

echo The number of files are $numFiles and the number of matching lines are $numLines
