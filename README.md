# TimeTracker
A quick and dirty command line tool to track how long you've been working. 

The program utilizes a diary system to keep track of task names and how long they've been worked on, one entry per line in a file.

## Installation
After pulling the git repo, installation is just via `make all` or `make`. Alternatively, you can run `gcc -o *executable name* track.c`. 


## Running
All commands are invoked via `./track <flags>`. For a list of all flags, run `./track -help` or `./track -h`.

The `-f <filename>` flag allows for changing of "diaries". The default diary is located at time.txt in the invocation directory. That is, when normally calling 
commands, the diary will be stored at "./time.txt", for any directory you are in. 
It's thus advantageous to set the flag to a safe place, or else you may end up working with multiple diaries.

The `-s` flag allows  to start a task. Use `-t <taskname>` to set a task name. So via the command `./track -s -t "Testing"`, we begin a new task called testing, 
stored in the local diary.

The `-e` flag ends the first ongoing task it can find. It will also output how long the task went on for.

The `-d` flag prints out how long the most recent task has been going on for.

The `-p` flag prints out all of the most recent tasks, within the last 13 days by default. This value can be changed via the `-n <number>` flag. So `./track -p 
-n 8` will print all tasks within the last (8*24) hours.

