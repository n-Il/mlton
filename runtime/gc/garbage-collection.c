/* Copyright (C) 2009-2010,2012,2016 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a HPND-style license.
 * See the file MLton-LICENSE for details.
 */

void minorGC (GC_state s) {
  minorCheneyCopyGC (s);
}

void majorGC (GC_state s, size_t bytesRequested, bool mayResize) {
  uintmax_t numGCs;
  size_t desiredSize;

  s->lastMajorStatistics.numMinorGCs = 0;
  numGCs = 
    s->cumulativeStatistics.numCopyingGCs 
    + s->cumulativeStatistics.numMarkCompactGCs;
  if (0 < numGCs
      and ((float)(s->cumulativeStatistics.numHashConsGCs) / (float)(numGCs)
           < s->controls.ratios.hashCons))
    s->hashConsDuringGC = TRUE;
  desiredSize = 
    sizeofHeapDesired (s, s->lastMajorStatistics.bytesLive + bytesRequested, 0);
  if (not FORCE_MARK_COMPACT
      and not s->hashConsDuringGC // only markCompact can hash cons
      and s->heap.withMapsSize < s->sysvals.ram
      and (not isHeapInit (&s->secondaryHeap)
           or createHeapSecondary (s, desiredSize)))
    majorCheneyCopyGC (s);
  else
    majorMarkCompactGC (s);
  s->hashConsDuringGC = FALSE;
  s->lastMajorStatistics.bytesLive = s->heap.oldGenSize;
  if (s->lastMajorStatistics.bytesLive > s->cumulativeStatistics.maxBytesLive)
    s->cumulativeStatistics.maxBytesLive = s->lastMajorStatistics.bytesLive;
  /* Notice that the s->lastMajorStatistics.bytesLive below is
   * different than the s->lastMajorStatistics.bytesLive used as an
   * argument to createHeapSecondary above.  Above, it was an
   * estimate.  Here, it is exactly how much was live after the GC.
   */
  if (mayResize) {
    resizeHeap (s, s->lastMajorStatistics.bytesLive + bytesRequested);
  }
  setCardMapAndCrossMap (s);
  resizeHeapSecondary (s);
  assert (s->heap.oldGenSize + bytesRequested <= s->heap.size);
  

  //Heap Profiling Code
  if (s->heapProfilingFile != NULL){ 
    //rusage is used to gather ms since program started execution
    struct rusage ru_hprofiling;
    uintmax_t time_hprofiling;
    getrusage(RUSAGE_SELF, &ru_hprofiling);
    time_hprofiling = rusageTime (&ru_hprofiling);

    //Write the time,live bytes, and heap size in bytes to the file
    fwrite(&time_hprofiling,sizeof(uintmax_t),1,s->heapProfilingFile);
    fwrite(&s->heap.oldGenSize,sizeof(size_t),1,s->heapProfilingFile);//same as lastMajorStatistics.bytesLive
    fwrite(&s->heap.size,sizeof(size_t),1,s->heapProfilingFile);

    //start of code to traverse heap 
    pointer back;
    pointer front;
    GC_header header;
    GC_header *headerp;
    pointer p;
    size_t size;
    front = alignFrontier (s, s->heap.start);
    back = s->heap.start + s->heap.oldGenSize;
    int mods[] = {0,0,0,0,0};
    // 1,2,lt10, lt50, gte50
    int survives[] = {0,0,0,0,0};
    updateObject:
    //end condition
    if (front == back)
        goto done;
    p = advanceToObjectData (s, front);
    headerp = getHeaderp (p);
    header = *headerp;
    if (GC_VALID_HEADER_MASK & header){
        GC_header higher32mask = (GC_header)0xFFFFFFFF00000000;
        GC_header lower32mask = (GC_header)0x00000000FFFFFFFF;
        GC_header higher32 = (higher32mask & header) >> 32;
        if (higher32 != 0)
        {
            printf("headertop32 = 0x%" PRIx64 "\n",higher32);
        }
        if (s->heapProfilingGcSurvived){ 
            //increase unless increasing would run out of space
            if (higher32 < 4294967295){
                GC_header newheader =  (((higher32 + 1) << 32) | (lower32mask & header));
                *headerp = newheader;
                //logging data
                GC_header newhigher32 = higher32+1;
                if (newhigher32 == 1){
                    survives[0]++; 
                }else if (newhigher32 == 2){
                    survives[1]++; 
                }else if (newhigher32 < 10){
                    survives[2]++; 
                }else if (newhigher32 < 50){
                    survives[3]++; 
                }else{
                    survives[4]++;
                }
            }else{
                printf("Heap Profiling hitting max value for gc survived\n");
                survives[4]++;
            }


        }else{
            int randomModFive = higher32 % 5;
            //printf("header = 0x%" PRIx64 "\n",header);
            //printf("headertop32 = 0x%" PRIx64 "\n",higher32);
            //printf("mod5 = %d\n",randomModFive);
            if (randomModFive == 0){
                mods[0]++;
            } else if (randomModFive == 1){
                mods[1]++;
            } else if (randomModFive == 2){
                mods[2]++;
            } else if (randomModFive == 3){
                mods[3]++;
            } else if (randomModFive == 4){
                mods[4]++;
            }
        }
    }else{
        printf("unexpected header at heap profiling code");
    }
    size = sizeofObject (s, p);
    front += size;
    goto updateObject;
    done:
    if (s->heapProfilingGcSurvived){
        fwrite(&survives[0],sizeof(int),1,s->heapProfilingFile);
        fwrite(&survives[1],sizeof(int),1,s->heapProfilingFile);
        fwrite(&survives[2],sizeof(int),1,s->heapProfilingFile);
        fwrite(&survives[3],sizeof(int),1,s->heapProfilingFile);
        fwrite(&survives[4],sizeof(int),1,s->heapProfilingFile);
    }else{ 
        fwrite(&mods[0],sizeof(int),1,s->heapProfilingFile);
        fwrite(&mods[1],sizeof(int),1,s->heapProfilingFile);
        fwrite(&mods[2],sizeof(int),1,s->heapProfilingFile);
        fwrite(&mods[3],sizeof(int),1,s->heapProfilingFile);
        fwrite(&mods[4],sizeof(int),1,s->heapProfilingFile);
    }
    //end of code to traverse heap 
  }
}

void growStackCurrent (GC_state s) {
  size_t reserved;
  GC_stack stack;

  reserved = sizeofStackGrowReserved (s, getStackCurrent(s));
  if (DEBUG_STACKS or s->controls.messages)
    fprintf (stderr, 
             "[GC: Growing stack of size %s bytes to size %s bytes, using %s bytes.]\n",
             uintmaxToCommaString(getStackCurrent(s)->reserved),
             uintmaxToCommaString(reserved),
             uintmaxToCommaString(getStackCurrent(s)->used));
  assert (hasHeapBytesFree (s, sizeofStackWithMetaData (s, reserved), 0));
  stack = newStack (s, reserved, TRUE);
  copyStack (s, getStackCurrent(s), stack);
  getThreadCurrent(s)->stack = pointerToObjptr ((pointer)stack, s->heap.start);
  markCard (s, objptrToPointer (getThreadCurrentObjptr(s), s->heap.start));
}

void enterGC (GC_state s) {
  if (s->profiling.isOn) {
    /* We don't need to profileEnter for count profiling because it
     * has already bumped the counter.  If we did allow the bump, then
     * the count would look like function(s) had run an extra time.
     */  
    if (s->profiling.stack
        and not (PROFILE_COUNT == s->profiling.kind))
      GC_profileEnter (s);
  }
  s->amInGC = TRUE;
}

void leaveGC (GC_state s) {
  if (s->profiling.isOn) {
    if (s->profiling.stack
        and not (PROFILE_COUNT == s->profiling.kind))
      GC_profileLeave (s);
  }
  s->amInGC = FALSE;
}

void performGC (GC_state s, 
                size_t oldGenBytesRequested,
                size_t nurseryBytesRequested, 
                bool forceMajor,
                bool mayResize) {
  uintmax_t gcTime;
  bool stackTopOk;
  size_t stackBytesRequested;
  struct rusage ru_start;
  size_t totalBytesRequested;

  enterGC (s);
  s->cumulativeStatistics.numGCs++;
  if (DEBUG or s->controls.messages) {
    size_t nurserySize = s->heap.size - ((size_t)(s->heap.nursery - s->heap.start));
    size_t nurseryUsed = (size_t)(s->frontier - s->heap.nursery);
    fprintf (stderr, 
             "[GC: Starting gc #%s; requesting %s nursery bytes and %s old-gen bytes,]\n",
             uintmaxToCommaString(s->cumulativeStatistics.numGCs),
             uintmaxToCommaString(nurseryBytesRequested),
             uintmaxToCommaString(oldGenBytesRequested));
    fprintf (stderr, 
             "[GC:\theap at "FMTPTR" of size %s bytes (+ %s bytes card/cross map),]\n",
             (uintptr_t)(s->heap.start),
             uintmaxToCommaString(s->heap.size),
             uintmaxToCommaString(s->heap.withMapsSize - s->heap.size));
    fprintf (stderr, 
             "[GC:\twith old-gen of size %s bytes (%.1f%% of heap),]\n",
             uintmaxToCommaString(s->heap.oldGenSize),
             100.0 * ((double)(s->heap.oldGenSize) / (double)(s->heap.size)));
    fprintf (stderr,
             "[GC:\tand nursery of size %s bytes (%.1f%% of heap),]\n",
             uintmaxToCommaString(nurserySize),
             100.0 * ((double)(nurserySize) / (double)(s->heap.size)));
    fprintf (stderr,
             "[GC:\tand nursery using %s bytes (%.1f%% of heap, %.1f%% of nursery).]\n",
             uintmaxToCommaString(nurseryUsed),
             100.0 * ((double)(nurseryUsed) / (double)(s->heap.size)),
             100.0 * ((double)(nurseryUsed) / (double)(nurserySize)));
  }
  assert (invariantForGC (s));
  if (needGCTime (s))
    startTiming (&ru_start);
  minorGC (s);
  stackTopOk = invariantForMutatorStack (s);
  stackBytesRequested = 
    stackTopOk 
    ? 0 
    : sizeofStackWithMetaData (s, sizeofStackGrowReserved (s, getStackCurrent (s)));
  totalBytesRequested = 
    oldGenBytesRequested 
    + nurseryBytesRequested
    + stackBytesRequested;
  if (forceMajor 
      or totalBytesRequested > s->heap.size - s->heap.oldGenSize)
    majorGC (s, totalBytesRequested, mayResize);
  setGCStateCurrentHeap (s, oldGenBytesRequested + stackBytesRequested, 
                         nurseryBytesRequested);
  assert (hasHeapBytesFree (s, oldGenBytesRequested + stackBytesRequested,
                            nurseryBytesRequested));
  unless (stackTopOk)
    growStackCurrent (s);
  setGCStateCurrentThreadAndStack (s);
  if (needGCTime (s)) {
    gcTime = stopTiming (&ru_start, &s->cumulativeStatistics.ru_gc);
    s->cumulativeStatistics.maxPauseTime = 
      max (s->cumulativeStatistics.maxPauseTime, gcTime);
  } else
    gcTime = 0;  /* Assign gcTime to quell gcc warning. */
  if (DEBUG or s->controls.messages) {
    size_t nurserySize = s->heap.size - (size_t)(s->heap.nursery - s->heap.start);
    fprintf (stderr, 
             "[GC: Finished gc #%s; time %s ms,]\n",
             uintmaxToCommaString(s->cumulativeStatistics.numGCs),
             uintmaxToCommaString(gcTime));
    fprintf (stderr, 
             "[GC:\theap at "FMTPTR" of size %s bytes (+ %s bytes card/cross map),]\n",
             (uintptr_t)(s->heap.start),
             uintmaxToCommaString(s->heap.size),
             uintmaxToCommaString(s->heap.withMapsSize - s->heap.size));
    fprintf (stderr, 
             "[GC:\twith old-gen of size %s bytes (%.1f%% of heap),]\n",
             uintmaxToCommaString(s->heap.oldGenSize),
             100.0 * ((double)(s->heap.oldGenSize) / (double)(s->heap.size)));
    fprintf (stderr,
             "[GC:\tand nursery of size %s bytes (%.1f%% of heap).]\n",
             uintmaxToCommaString(nurserySize),
             100.0 * ((double)(nurserySize) / (double)(s->heap.size)));
  }
  /* Send a GC signal. */
  if (s->signalsInfo.gcSignalHandled
      and s->signalHandlerThread != BOGUS_OBJPTR) {
    if (DEBUG_SIGNALS)
      fprintf (stderr, "GC Signal pending.\n");
    s->signalsInfo.gcSignalPending = TRUE;
    unless (s->signalsInfo.amInSignalHandler) 
      s->signalsInfo.signalIsPending = TRUE;
  }
  if (DEBUG) 
    displayGCState (s, stderr);
  assert (hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested));
  assert (invariantForGC (s));
  leaveGC (s);
}

void ensureInvariantForMutator (GC_state s, bool force) {
  if (force
      or not (invariantForMutatorFrontier(s))
      or not (invariantForMutatorStack(s))) {
    /* This GC will grow the stack, if necessary. */
    performGC (s, 0, getThreadCurrent(s)->bytesNeeded, force, TRUE);
  }
  assert (invariantForMutatorFrontier(s));
  assert (invariantForMutatorStack(s));
}

/* ensureHasHeapBytesFree (s, oldGen, nursery) 
 */
void ensureHasHeapBytesFree (GC_state s, 
                             size_t oldGenBytesRequested,
                             size_t nurseryBytesRequested) {
  assert (s->heap.nursery <= s->limitPlusSlop);
  assert (s->frontier <= s->limitPlusSlop);
  if (not hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested))
    performGC (s, oldGenBytesRequested, nurseryBytesRequested, FALSE, TRUE);
  assert (hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested));
}

void GC_collect (GC_state s, size_t bytesRequested, bool force) {
  enter (s);
  /* When the mutator requests zero bytes, it may actually need as
   * much as GC_HEAP_LIMIT_SLOP.
   */
  if (0 == bytesRequested)
    bytesRequested = GC_HEAP_LIMIT_SLOP;
  getThreadCurrent(s)->bytesNeeded = bytesRequested;
  switchToSignalHandlerThreadIfNonAtomicAndSignalPending (s);
  ensureInvariantForMutator (s, force);
  assert (invariantForMutatorFrontier(s));
  assert (invariantForMutatorStack(s));
  leave (s);
}
