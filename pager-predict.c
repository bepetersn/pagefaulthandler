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

#define PAGE_LIMIT_PER_PROCESS 5
#define AVG_PROCESS_MAX_PAGE 11

int predict_next_page(Pentry p, int page, int timestamps[MAXPROCPAGES])
{
    UNUSED(p);
    UNUSED(timestamps);
    if (page < AVG_PROCESS_MAX_PAGE)
    {
        return page + 1;
    }
    else
    {
        return 0;
    }
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

int num_pages_swapping_in_right_now(Pentry p, int proc) {
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

int find_LRU_victim(Pentry p, int page, int timestamps[MAXPROCPAGES])
{
    // Try to find the LRU page; this should
    // be the page with the lowest value in its
    // timestamp that is neither 0 nor the one we want,
    // and which is still in memory
    int lru_page = page;
    int pagetmp = 0;
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

void handle_swap_in(Pentry p, int proc, int page, int timestamps[MAXPROCPAGES])
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

                if(pageout(proc, // NOTE: May fail if this page is already
                        // in the process of being swapped in
                        find_LRU_victim(p, page, timestamps))) {

                    // printf("o ");
                } else {
                    // printf("*i ");
                }
            } else {
                // printf("i ");
            }
        }
        else if (pages_used_now == PAGE_LIMIT_PER_PROCESS)
        {
            if(pageout(proc, // NOTE: May fail if this page is already
                        // in the process of being swapped in
                        find_LRU_victim(p, page, timestamps))) {

                    // printf("o ");
            } else {
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

    /* Local vars */
    int proctmp;
    int pagetmp;
    int proc;
    int page;
    int pc;
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
            
            handle_swap_in(p, proc, page, timestamps[proc]);
            // TODO: check for previous prediction miss
            handle_swap_in(p, proc, predict_next_page(p, page, timestamps[proc]),
                           timestamps[proc]);
        }
    }
    /* advance time for next pageit iteration */
    tick++;
}
