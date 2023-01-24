#!/bin/bash

# File Name: writer.sh
# File Description: This script creates a file at the desired location and write the string into it (AESD Assignment 1)
# Author :Ayswariya Kannan 

#Defining the input arguments
writefile=$1
writestr=$2

# Checking if valid no of arguments inserted
if [ $# -ne 2 ]
then
    echo "ERROR: Invalid No of arguments"
    echo "Specify the file path and the string to be inserted"
    exit 1

 
else 
     # create the directory variable
    dir_name=$(dirname "${writefile}")
  

    # check if directory exists, else create a new directory
    if [ ! -d "$writefile" ]
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

