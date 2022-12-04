#!/bin/bash

path=$1
text=$2

if [ $# -lt 2 ]
then
    echo Must provide 2 arguments
    exit 1
fi

mkdir -p "${path%/*}" && echo $text > $path
