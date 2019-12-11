// translate.h
//	Data structures for managing the translation from
//	virtual page # -> physical page #, used for managing
//	physical memory on behalf of user programs.
//
//	The data structures in this file are "dual-use" - they
//	serve both as a page table entry, and as an entry in
//	a software-managed translation lookaside buffer (TLB).
//	Either way, each entry is of the form:
//	<virtual page #, physical page #>.
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef TLB_H
#define TLB_H

#include "copyright.h"
#include "utility.h"

// The following class defines an entry in a translation table -- either
// in a page table or a TLB.  Each entry defines a mapping from one
// virtual page to one physical page.
// In addition, there are some extra bits for access control (valid and
// read-only) and some bits for usage information (use and dirty).

class TranslationEntry
{
public:
    int virtualPage;  // The page number in virtual memory.
    int physicalPage; // The page number in real memory (relative to the
                      //  start of "mainMemory"
    bool valid;       // If this bit is set, the translation is ignored.
                      // (In other words, the entry hasn't been initialized.)
    bool readOnly;    // If this bit is set, the user program is not allowed
                      // to modify the contents of the page.
    bool use;         // This bit is set by the hardware every time the
                      // page is referenced or modified.
    bool dirty;       // This bit is set by the hardware every time the
                      // page is modified.
    int count;        //用来计数，在替换过程中会根据该值进行替换。

    bool onDisk; //当前页面是否在磁盘上，和在磁盘上的地址
    int diskAddr;
    bool codeData;  // 是否为存储代码和数据的页面
    TranslationEntry(){
        virtualPage = -1;
        physicalPage = -1;
        valid = FALSE;
        readOnly = FALSE;
        use = FALSE;
        dirty = FALSE;
        count = 0;
        onDisk = FALSE;
        diskAddr = 0;
        codeData = FALSE;
    }
};

#endif
