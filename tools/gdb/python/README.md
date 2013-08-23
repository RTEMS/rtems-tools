# RTEMS GDB

GDB extensions to help accelarting RTEMS debugging.

## Usage
 - Clone the git repo
 - Fire up gdb and use source command

```
$ sparc-rtems4.11-gdb

GNU gdb (GDB) 7.5.1
Copyright (C) 2012 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "--host=x86_64-linux-gnu --target=sparc-rtems4.11".
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
(gdb) source path/to/clone/__init__.py
RTEMS GDB Support loaded
(gdb)
```

## Commands Implemented
 - `rtems object` : Prints rtems objects by ID
 - [rtems index subcommands](Subcommands)

## Developer documentation
We have a document to get started with [pretty printer development](Writing-a-pretty-printer).

