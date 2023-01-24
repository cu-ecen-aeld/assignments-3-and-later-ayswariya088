#!/bin/bash
# File Name: finder.sh
# File Description: This script checks whether the required string exists inside the given file directory
# Author :Ayswariya Kannan 

#Defining the input Arguments 
filesdir=$1
searchstr=$2

# condition to check for a valid no of arguments
if [ $# -ne 2 ]
then
    echo "ERROR :Invalid No. of arguments"
    echo "Specify path of the directory and the string to be searched"
    exit 1

# condition to check whether the file directory actually exists
else 
    if [ -d "$filesdir" ]
        then  #if it exists search for the string

                # Counting number of lines that have the string
                # -r recursive search
                # wc -l cmd counts no of lines in the output
                n_line=$(grep -r ${searchstr} ${filesdir} | wc -l)

                # Counting number of files that have the string
                n_file=$(grep -lr ${searchstr} ${filesdir} | wc -l)
                
                echo "The number of files are $n_file and the number of matching lines are $n_line"
                exit 0

# Error printed in case the path is not found   
else
    echo " ERROR: $filesdir does not represent a directory on the filesystem"
    exit 1
    
fi
fi

