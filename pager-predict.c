/*
 * File: pager-predict.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 * 	This file contains a predictive pageit
 *      implmentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include "simulator.h"

#define UNUSED(x) (void)(x)

#define LAST 1
#define CURRENT 0
#define PAGE_LIMIT_PER_PROCESS 5
#define AVG_PROCESS_MAX_PAGE 11

int classify(int cycle)
{
    // if (pcs[CURRENT] > 1682) // doesn't work for some reasons
    //     return 3;
    switch (cycle)
    {
    case 1533: // We get these off-by-1 jumps when we detect cycles at every step
    case 1534:
        return 0;
    case 1129: // Same
    case 1130:
        return 1;
    case 516:
    case 1683:
        return 2;
    case 503: // Same
    case 504:
    case 501:
        return 4;
    case 1911:
        // You wouldn't think going to the end of its program
        // would count as a cycle, but from our perspective
        // the next program starts right afterward, at 0
        return -1;
    default:
        printf("cycle of %d detected", cycle);
        exit(EXIT_FAILURE);
    }
}

int detect_cycle(int last_pc, int pc)
{
    /* Detect the innermost cycle made by the pc for this 
       process; this could be from going back to the beginning 
       of a for-loop, or going to a label */

    // After some instructions have executed,
    if (last_pc != -1)
    {
        if (pc == last_pc)
        {
            return 0; // noop
        }
        else
        {
            if (pc > last_pc)
            {
                return 0; // instruction execution
            }
            else
            { // (pc < last_pc) -- cycle occurred
                return last_pc - pc;
            }
        }
    }
    return 0;
}

int predict_next_page(Pentry p, int page, int timestamps[MAXPROCPAGES], int proc_type)
{
    UNUSED(p);
    UNUSED(timestamps);
    UNUSED(proc_type);
    if (proc_type != -1)
    {
        switch(proc_type) {
            case 0:
                if(page == 3) {  // P0 makes this jump, probability: 0.6
                    return 10;
                }
                if(page == 12) { // P0 makes this jump, sometimes; should be rarer than it is?
                    return 0;
                }
                break;
            case 1:
                if (page < 9) { // max page requested by P1
                    return page + 1;
                } else {
                    return 0;
                }
            case 2: 
                if (page == 12) { // P2 makes this jump, typically
                    return 9;
                }
                break;
            case 4:
                if (page < 4) { // max page requested by P4
                    return page + 1;
                } else {
                    return 0;
                }
        }
    } 
    // best global strategy we have
    if (page < AVG_PROCESS_MAX_PAGE)
    {
        return page + 1;
    }
    return 0;
}

int num_pages_swapped_in_right_now(Pentry p)
{
    int num_pages = 0;
    for (int i = 0; i < p.npages; i++)
    {
        if (p.pages[i])
        {
            // printf("%d", num_pages);
            num_pages++;
        }
    }
    return num_pages;
}

int num_pages_swapping_in_right_now(Pentry p, int proc)
{
    int num_pages = 0;
    for (int i = 0; i < p.npages; i++)
    {
        // If a page is not current paged in, we can call pageout
        // idempotently to no effect, EXCEPT this will return 0 when
        // the page is currently swapping in -- therefore, we can
        // tally how many are swapping in right now
        if (!p.pages[i] && !pageout(proc, i))
        {
            // printf("%d", num_pages);
            num_pages++;
        }
    }
    return num_pages;
}

int find_LRU_victim(Pentry p, int page, int timestamps[MAXPROCPAGES], int proc_type)
{
    // Try to find the LRU page; this should
    // be the page with the lowest value in its
    // timestamp that is neither 0 nor the one we want,
    // and which is still in memory
    int lru_page = page;
    int pagetmp = 0;
    UNUSED(proc_type);
    for (pagetmp = 0; pagetmp < p.npages; pagetmp++)
    {
        // Find the victim: mininum non-zero
        // timestamp that is in mem
        if (timestamps[pagetmp] &&
            (timestamps[pagetmp] <
             timestamps[lru_page]) &&
            pagetmp != page &&
            p.pages[pagetmp])
        {
            lru_page = pagetmp;
        }
    }
    return lru_page;
}

void handle_swap_in(Pentry p, int proc, int page, int timestamps[MAXPROCPAGES], int proc_type)
{
    /* Is page out of memory? Try to page something in; 
       If this fails, page something out */
    int pagein_required = !p.pages[page];
    if (pagein_required)
    {

        // Keep track of total pages used so we don't swap in too many
        int pages_pagedin_now = num_pages_swapped_in_right_now(p);
        int pages_pagingin_now = num_pages_swapping_in_right_now(p, proc);
        int pages_used_now = pages_pagedin_now + pages_pagingin_now;
        if (pages_used_now < PAGE_LIMIT_PER_PROCESS)
        {
            if (!pagein(proc, page)) // NOTE: May fail if there are no physical frames available
                                     //       OR if this page is in the process of being swapped out
            {

                if (pageout(proc, // NOTE: May fail if this page is already
                            // in the process of being swapped in
                            find_LRU_victim(p, page, timestamps, proc_type)))
                {

                    // printf("o ");
                }
                else
                {
                    // printf("*i ");
                }
            }
            else
            {
                // printf("i ");
            }
        }
        else if (pages_used_now == PAGE_LIMIT_PER_PROCESS)
        {
            if (pageout(proc, // NOTE: May fail if this page is already
                        // in the process of being swapped in
                        find_LRU_victim(p, page, timestamps, proc_type)))
            {

                // printf("o ");
            }
            else
            {
                // printf("*i ");
            }
        }
        else
        {
            printf("This process got too many pages: %d, %d\n", proc, pages_pagedin_now);
            exit(EXIT_FAILURE);
        }
    }
}

void pageit(Pentry q[MAXPROCESSES])
{
    /* Static vars */
    static int initialized = 0;
    static int tick = 1;
    static int timestamps[MAXPROCESSES][MAXPROCPAGES];
    static int pcs[MAXPROCESSES][2];
    static int proc_types[MAXPROCESSES];

    /* Local vars */
    int proctmp;
    int pagetmp;
    int proc;
    int page;
    int pc;
    int cycle;
    int next_page;
    int proc_type;
    Pentry p;

    /* initialize static vars on first run */
    if (!initialized)
    {
        for (proctmp = 0; proctmp < MAXPROCESSES; proctmp++)
        {
            for (pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++)
            {
                timestamps[proctmp][pagetmp] = 0;
            }
            pcs[proctmp][CURRENT] = -1;
            proc_types[proctmp] = -1;
        }
        initialized = 1;
    }

    // printf("\n t \n");
    // fflush(stdout);

    /* Loop through each process */
    for (proc = 0; proc < MAXPROCESSES; proc++)
    {
        /* Is process active? */
        p = q[proc];
        if (p.active)
        {
            pc = p.pc;            // program counter for process
            page = pc / PAGESIZE; // page the program counter needs

            // Note this proc's page as having been used this tick
            timestamps[proc][page] = tick;
            // Record the last and current PCs
            pcs[proc][LAST] = pcs[proc][CURRENT];
            pcs[proc][CURRENT] = pc;

            // Detect innermost cycle and classify
            // if (proc_types[proc] == -1) // When we do this only once, we mess up on second round
            // {
            cycle = detect_cycle(pcs[proc][LAST], pc);
            if (cycle)
            {
                proc_types[proc] = classify(cycle);
                // printf("%d, %d\n", proc, proc_types[proc]);
                // fflush(stdout);
            }
            proc_type = proc_types[proc];
            // }

            if(proc == 5) {
                if(proc < -2) {
                    return;
                }
            }
            handle_swap_in(p, proc, page, timestamps[proc], proc_type);
            // TODO: check for previous prediction miss
            next_page = predict_next_page(p, page, timestamps[proc], proc_type);
            handle_swap_in(p, proc, next_page, timestamps[proc], proc_type);
        }
    }
    /* Advance time for next pageit iteration */
    tick++;
    // getchar();
    /* Record the last pc */
}
