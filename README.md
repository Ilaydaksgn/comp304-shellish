# COMP304 — Shell-ish Implementation

GitHub Repository:
https://github.com/Ilaydaksgn/comp304-shellish
The project is implemented and tested on Linux (Ubuntu).

## Build Instructions

Compile the shell using gcc:

bash
gcc -Wall -Wextra -o shell-ish shell-ish-skeleton.c cut_builtin.c chatroom_builtin.c pstat_builtin.c

## Running the Shell

bash
./shell-ish

## Implemented Features

### Part I — Basic Shell Execution

This part includes:

* Fork and exec system calls
* PATH search for commands
* Argument parsing
* Running external programs

Example:

bash
ls -l
pwd
echo hello


### Part II — Background Execution

Commands can run in the background using &.

Example:

bash
sleep 5 &


### Part III — I/O Redirection

The shell supports input and output redirection.

Supported operators:

* <  input redirection
* >  output overwrite
* >> output append

Examples:

bash
cat < input.txt
echo hello > out.txt
echo world >> out.txt

### Part IV — Pipes

Command pipelines are supported using |.

Examples:

bash
ls | grep txt
cat file.txt | wc -l
ps aux | grep bash

## Built-in Commands

###1️1  cut

Extract selected fields from input similar to Unix cut.

Usage:

bash
cut -d<delimiter> -f<field_list>

Options:

* -d delimiter character
* -f field list (1-based indices)
### 2 chatroom

A simple multi-user chat system implemented using named pipes (FIFO) in /tmp.

Each chatroom corresponds to a directory:


/tmp/chatroom-<roomname>


Usage:

bash
chatroom <roomname> <username>


Example:

bash
chatroom comp304 ilayda


To exit chatroom:


Ctrl + C


Multiple terminals can join the same room with different usernames.

---

### 3 pstat

Displays process information using the Linux /proc filesystem.

Usage:

bash
pstat <pid>


* Command name
* Process state
* Parent PID
* Thread count
* Memory usage (VmSize, VmRSS, etc.)
* UID / GID information

### 4 Custom Command

Additional functionality was implemented through built-in commands (cut, chatroom, pstat) to extend shell capabilities beyond basic execution.

## Screenshots

Screenshots demonstrating implemented features are provided in the imgs/ folder, including:

* Basic command execution
* Background processes
* Redirection
* Pipes
* Built-in commands
* Chatroom communication
* pstat output

## Supplementary Files

The repository includes:

* Source files (.c)
* README
* imgs folder (screenshots)

## Notes

* The shell is designed to run on Linux systems.
* Some parsing behaviors depend on input formatting.
* Named pipes are stored in /tmp and removed automatically by the system when cleaned.

Ilayda Kesgin 
