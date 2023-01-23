#!/bin/bash

# file: finder.sh
# This script checks whether the required string exists inside the given file directory
# AESD Assignment-1
#Input arguments
filesdir=$1
searchstr=$2

# Checking for a valid number of arguments
if [ $# -ne 2 ]
then
    echo "Invalid No. of arguments"
    exit 1

# Search whether the file directory actually exists
elif [ -d "$filesdir" ]
then  #if it exists search for the string

    # Counting number of lines that have the string
    n_line=$(grep -r ${searchstr} ${filesdir} | wc -l)

    # Counting number of files that have the string
    n_file=$(grep -lr ${searchstr} ${filesdir} | wc -l)
    
    echo "The number of files are $n_file and the number of matching lines are $n_line"
    exit 0

# Error printed in case the path is not found   
else
    echo '$filesdir does not represent a directory on the filesystem'
    exit 1
    
fi

