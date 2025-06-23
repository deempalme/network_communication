#!/bin/bash

# Defining colors
blue='\e[38;5;51m'
green='\e[38;5;118m'
red='\e[0;91m'
yellow='\e[0;93m'
## Clear all formatting
clear='\e[0;0m'

### Print a BLUE colored message in terminal
###
### This does not add a newline character at the end of the message
###
### Example of use:
###     print_blue "Message to print in blue"
###
### @param $1  Message to print
###
print_blue(){
    echo -e -n "${blue}$1${clear}"
}

### Print a GREEN colored message in terminal
###
### This does not add a newline character at the end of the message
###
### Example of use:
###     print_green "Message to print in green"
###
### @param $1  Message to print
###
print_green(){
    echo -e -n "${green}$1${clear}"
}

### Print a RED colored message in terminal
###
### This does not add a newline character at the end of the message
###
### Example of use:
###     print_red "Message to print in red"
###
### @param $1  Message to print
###
print_red(){
    echo -e -n "${red}$1${clear}"
}

### Print a YELLOW colored message in terminal
###
### This does not add a newline character at the end of the message
###
### Example of use:
###     print_yellow "Message to print in yellow"
###
### @param $1  Message to print
###
print_yellow(){
    echo -e -n "${yellow}$1${clear}"
}

### Clear all previous formatting
clear_format(){
    echo -e -n "$clear"
}