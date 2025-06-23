#!/bin/bash

# ::::::::::: LOG STYLIZER :::::::::::

. $(dirname "$0")/log.sh

# ::::::::: HELPER FUNCTIONS :::::::::

print_message(){
  print_green ">>>>>>>>>>>>>>>>>>> $1...\n\n"
}

check_error(){
  if [[ "$1" -ne '0' ]]; then
    print_red "\n<<<<<<<<<<<<<<<< There was an error ($1)...\n\n"
    exit
  fi
}

project_name="RamRodTcpNetwork"

# Creates the build directory just in case it does not exist
print_message "Creating directory"
mkdir -p /tmp/$project_name

# Running CMake
print_message "Configuring $project_name"
cmake -B /tmp/$project_name -DCMAKE_BUILD_TYPE:String=Debug -DCMAKE_C_COMPILER:String=/usr/bin/gcc -DCMAKE_CXX_COMPILER:String=/usr/bin/g++;
check_error $?

print_message "Compiling $project_name"
cd /tmp/$project_name
cmake --build . --target all -j $(nproc)
check_error $?

print_message "Executing $project_name"
./$project_name
check_error $?
