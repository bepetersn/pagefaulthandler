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
#define MAX_PAGE_USED 15

#define proc_log(proc, msg)                \
    {                                      \
        if (proc == 5)                     \
        {                                  \
            printf("process =5: %s", msg); \
            fflush(stdout);                \
        }                                  \
    }

int classify(int jump)
{
    switch (jump)
    {
    case -1533: // We get these similar-but-off-by-1 jumps when we detect at every step
    case -1534:
    case 902:
    case 132:
        return 0;
    case -1129: // Same
    case -1130:
        return 1;
    case -516:
    case -1683:
        return 2;
    case -503: // Same
    case -504:
    case -501:
    case 3:
        return 4;
    case -1911:
        // You wouldn't think going to the end of its program
        // would count as a jump, but from our perspective
        // the next program starts right afterward, at 0
        return -1;
    default:
        printf("jump of %d detected", jump);
        exit(EXIT_FAILURE);
    }
}

int detect_jump(int last_pc, int pc)
{
    /* Detect a jump made by the pc for this 
       process; this could be from going back to the beginning 
       of a for-loop, or going to a label, or skipping one branch
       of an if-statement */

    // After some instructions have executed,
    if (last_pc != -1)
    {
        if (pc == last_pc)
        {
            return 0; // noop
        }
        else
        {
            if ((pc - last_pc) == 1)
            {
                return 0; // instruction execution
            }
            else
            { // (pc - last_pc != 1) -- jump occurred
                return pc - last_pc;
            }
        }
    }
    return 0;
}

int predict_next_page(Pentry p, int page, int timestamps[MAXPROCPAGES], int proc_type)
{
    UNUSED(p);
    UNUSED(timestamps);
    if (proc_type != -1)
    {
        switch (proc_type)
        {
        case 0:
            if (page == 3)
            { // P0 makes this jump, probability: 0.6
                return 10;
            }
            if (page == 11)
            { // P0 makes this jump, sometimes; should be rarer than it is?
                return 0;
            }
            break;
        case 1:
            if (page < 9)
            { // max page requested by P1
                return page + 1;
            }
            else
            {
                return 0;
            }
        case 2:
            if (page == 12)
            { // P2 makes this jump, typically
                return 9;
            }
            break;
        case 4:
            if (page < 4)
            { // max page requested by P4
                return page + 1;
            }
            else
            {
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

int get_first_page_paged_in_right_now(Pentry p)
{
    int first_page = -1;
    for (int i = 0; i < p.npages; i++)
    {
        if (p.pages[i])
            first_page = i;
    }
    return first_page;
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

int page_is_swapping_in_right_now(Pentry p, int proc, int page) {
    for (int i = 0; i < p.npages; i++)
    {
        // If a page is not current paged in, we can call pageout
        // idempotently to no effect, EXCEPT this will return 0 when
        // the page is currently swapping in -- therefore, we can
        // tally how many are swapping in right now
        if (i == page && !p.pages[i] && !pageout(proc, i))
        {
            return 1;
        }
    }
    return 0;
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

int find_LRU_victim(Pentry p, int proc, int page, int timestamps[MAXPROCPAGES], int proc_type)
{
    // Try to find the LRU page; this should
    // be the page with the lowest value in its
    // timestamp that is neither 0 nor the one we want,
    // and which is still in memory

    // NOTE: lru_page may be -1; this will be sent to pageout
    // which can fail in that case

    UNUSED(proc_type);
    int lru_page = -1;
    int lru_timestamp = __INT_MAX__;
    int pagetmp = 0;
    for (pagetmp = 0; pagetmp < MAX_PAGE_USED; pagetmp++)
    {

        // Find the victim: mininum non-zero
        // timestamp that is in mem
        if (timestamps[pagetmp] &&
            (timestamps[pagetmp] <
             lru_timestamp) &&
            pagetmp != page &&
            p.pages[pagetmp])
        {
            lru_page = pagetmp;
            lru_timestamp = timestamps[pagetmp];
        }
    }
    if (proc == 5)
        printf("process= 5; victim: %d\n", lru_page);
    return lru_page;
}

void handle_swap_in(Pentry p, int proc, int page, int timestamps[MAXPROCPAGES], int proc_type)
{
    // if(proc == 5)
    //     printf("get page%d", page);

    /* Is page out of memory and not being paged in? 
       Try to page something in; 
       If this fails, page something out */
    int pagein_required = !p.pages[page];
    if (pagein_required && !page_is_swapping_in_right_now(p, proc, page))
    {

        // Keep track of total pages used so we don't swap in too many
        int pages_pagedin_now = num_pages_swapped_in_right_now(p);
        int pages_pagingin_now = num_pages_swapping_in_right_now(p, proc);
        int pages_used_now = pages_pagedin_now + pages_pagingin_now;
        // if(proc == 5) {
        //     printf("process= 5; pages_pagingin_now: %d\n", pages_pagingin_now);
        //     fflush(stdout);
        // }
        if (pages_used_now < PAGE_LIMIT_PER_PROCESS)
        {
            if (!pagein(proc, page))
            {

                if (pageout(proc, // NOTE: May fail if this page is already
                            // in the process of being swapped in
                            find_LRU_victim(p, proc, page, timestamps, proc_type)))
                {

                    // proc_log(proc, "o ");
                }
                else
                {
                    // proc_log(proc, "*i ");
                }
            }
            else
            {
                // There are no physical frames available
                // OR this page is in the process of being swapped out
            }
        }

        // Only proceed to free up space if we have not already over-allocated
        else if (pages_used_now == PAGE_LIMIT_PER_PROCESS)
        {
            // if(proc == 5)
            //     printf("process= 5: pagelimit\n");
            

            if (pageout(proc, // NOTE: May fail if this page is already
                        // in the process of being swapped in
                        find_LRU_victim(p, proc, page, timestamps, proc_type)))
            {

                // proc_log(proc, "o ");
            }
            else
            {
                // proc_log(proc, "*i ");
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
    int jump;
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
            printf("proc %d: %d, %d\n", proc, pc, pcs[proc][LAST]);

            // Detect any jumps and classify
            if(proc == 0) {
                printf("wtf");
            }
            jump = detect_jump(pcs[proc][LAST], pc);
            if (jump)
            {
                proc_types[proc] = classify(jump);
            }
            proc_type = proc_types[proc];

            // if (proc == 5)
            //     printf("process= 5; tick: %d; %d,", tick, page);

            handle_swap_in(p, proc, page, timestamps[proc], proc_type);
            // TODO: check for previous prediction miss
            next_page = predict_next_page(p, page, timestamps[proc], proc_type);
            
            // if (proc == 5) {
            //     printf("%d\n", next_page);
            //     fflush(stdout);
            // }

            handle_swap_in(p, proc, next_page, timestamps[proc], proc_type);
        }
    }
    /* Advance time for next pageit iteration */
    tick++;
    // getchar();
}
