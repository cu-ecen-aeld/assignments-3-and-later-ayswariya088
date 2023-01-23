#!/bin/bash

# file: writer.sh
# This script creates a file at the desired location and write the string into it
# AESD Assignment-1
#Input Arguments
writefile=$1
writestr=$2

# Checking if valid no of arguments inserted
if [ $# -ne 2 ]
then
    echo "Invalid No of arguments"
    exit 1

 
else 

    # create the directory variable
    dir_name=$(dirname "${writefile}")

    # check if directory exists, else create the directory
    if [ ! -d "$dir_name" ]
    then
        mkdir -p $dir_name
    fi

    # Write the given string into the write file
    echo $writestr > $writefile

    # check if file is created,if not print an error
    if [ -f "$writefile" ]
    then
        exit 0
    else
        echo "Error:Not Able to create file" 
        exit 1
    fi

fi
