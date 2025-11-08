# Assignment 5

Docs: [`docs/assignment 5.md`](./docs/assignment%205.md)

---

[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/9sszArbY)

---
As the final part of our OS development, we will try to alter the filesystem used by xv6. 

The mkfs program creates the xv6 file system disk image and determines how many total blocks the file system has. The format of an on-disk inode is defined by `struct dinode` in `fs.h`. You're particularly interested in `NDIRECT`, `NINDIRECT`, `MAXFILE`, and the `addrs[]` element of `struct dinode`. 

The code that finds a file's data on disk is in `bmap()` in `fs.c`. `bmap()` is called both when reading and writing a file. When writing, `bmap()` allocates new blocks as needed to hold file content, as well as allocating an indirect block if needed to hold block addresses.

`bmap()` deals with two kinds of block numbers. The `bn` argument is a "logical block number" - a block number within the file, relative to the start of the file. The block numbers in `ip->addrs[]`, and the argument to `bread()`, are disk block numbers. You can view `bmap()` as mapping a file's logical block numbers into disk block numbers.

1. Modify `bmap() `so that it implements a doubly-indirect block, in addition to direct blocks and a singly-indirect block. You'll have to have only 11 direct blocks, rather than 12, to make the new doubly-indirect block; you're not allowed to change the size of an on-disk inode. The first 11 elements of `ip->addrs[]` should be direct blocks; the 12th should be a singly-indirect block (just like the current one); the 13th should be your new doubly-indirect block. You are done with this when bigfile writes successfully. 

2. Add symbolic links to xv6. Symbolic links (or soft links) refer to a linked file or directory by pathname; when a symbolic link is opened, the kernel looks up the linked-to name. Symbolic links resemble hard links, but hard links are restricted to pointing to files on the same disk, cannot refer to directories, and are tied to a specific target i-node rather than (as with symbolic links) referring to whatever happens at the moment to be at the target name, if anything.
    
    1. You do not have to handle symbolic links to directories for this lab; the only system call that needs to know how to follow symbolic links is `open()`.
    2. You will implement the `symlink(char *target, char *path)` system call, which creates a new symbolic link at path that refers to file named by target. For further information, see the man page symlink.
