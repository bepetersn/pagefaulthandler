/*
 * File: pager-lru.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 * 	This file contains an lru pageit
 *      implmentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simulator.h"

void pageit(Pentry q[MAXPROCESSES])
{

    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES];

    /* Local vars */
    int proctmp;
    int pagetmp;
    int proc;
    int page;
    int pc;
    int lru_page;
    int proc_timestamps[MAXPROCPAGES];

    /* initialize static vars on first run */
    if (!initialized)
    {
        for (proctmp = 0; proctmp < MAXPROCESSES; proctmp++)
        {
            for (pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++)
            {
                timestamps[proctmp][pagetmp] = 0;
            }
        }
        initialized = 1;
    }

    /* Trivial paging strategy */
    /* Loop through each process */
    for (proc = 0; proc < MAXPROCESSES; proc++)
    {

        /* Is process active? */
        if (q[proc].active)
        {
            pc = q[proc].pc;      // program counter for process
            page = pc / PAGESIZE; // page the program counter needs
            // Note this proc's page as having been used this tick
            timestamps[proc][page] = tick;

            /* Is page swaped-out? */
            /* Try to swap in */
            // This can fail, but the best thing for us to
            // to do in that case is ignore it and wait and
            // try next tick
            if (!q[proc].pages[page] && !pagein(proc, page))
            {
                // Try to find the LRU page; this should
                // be the page with the lowest value in its
                // timestamp that is neither 0 nor the one we want,
                // and which is still in memory
                lru_page = page;
                // Make a copy of timestamps for
                // this process, for clarity
                memcpy(timestamps[proc],
                       proc_timestamps,
                       sizeof(int) * MAXPROCPAGES);
                for (pagetmp = 0; pagetmp < q[proc].npages; pagetmp++)
                {
                    if (proc_timestamps[pagetmp] &&
                        (proc_timestamps[pagetmp] <
                         proc_timestamps[lru_page]) &&
                        pagetmp != page &&
                        q[proc].pages[pagetmp])
                    {
                        lru_page = pagetmp;
                    }
                }
                pageout(proc, lru_page); // NOTE: May fail
            }
        }
    }
    /* advance time for next pageit iteration */
    tick++;
}