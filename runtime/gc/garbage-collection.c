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
    if (s->heapProfilingGcSurvived){ 
        if(s->heapProfilingGcSurvivedCounter < 2){
            //printf("increment:%d\n",s->heapProfilingGcSurvivedCounter);
            s->heapProfilingGcSurvivedCounter = s->heapProfilingGcSurvivedAccuracy; 
        } 
        else{
            //printf("skip:%d\n",s->heapProfilingGcSurvivedCounter);
            s->heapProfilingGcSurvivedCounter -- ;
        }
    }
    //rusage is used to gather ms since program started execution
    struct rusage ru_hprofiling;
    uintmax_t time_hprofiling;
    getrusage(RUSAGE_SELF, &ru_hprofiling);
    time_hprofiling = rusageTime (&ru_hprofiling);

    size_t object_count = 0;

    //lifetimes 1 2 3 4 5 <10 <100 <1000 <10000 <100000 <1000000 <10000000 and longer
    size_t survives[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
    size_t survivesSize[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};

    //locations
    //uint32_t numberNames = s->sourceMaps.sourceNamesLength; 
    uint32_t numberNames = s->sourceMaps.sourcesLength;//really numberSources
    size_t locationObjects[numberNames];
    size_t locationSize[numberNames];
    if (s->heapProfilingLocation){
        for(uint32_t i = 0;i < numberNames; numberNames++){
            locationObjects[i] = 0;
            locationSize[i] = 0;
        }
    }

    //start of code to traverse heap 
    pointer back;
    pointer front;
    GC_header header;
    GC_header *headerp;
    pointer p;
    size_t size;
    front = alignFrontier (s, s->heap.start);
    back = s->heap.start + s->heap.oldGenSize;
    updateObject:
    if (front == back)
        goto done;
    p = advanceToObjectData (s, front);
    headerp = getHeaderp (p);
    header = *headerp;
    size = sizeofObject (s, p);
    if (GC_VALID_HEADER_MASK & header){
        object_count++;
        GC_header higher32mask = (GC_header)0xFFFFFFFF00000000;
        GC_header lower32mask = (GC_header)0x00000000FFFFFFFF;
        GC_header higher32 = (higher32mask & header) >> 32;
        if (s->heapProfilingGcSurvived){ 
            //increase unless increasing would run out of space
            if(s->heapProfilingGcSurvivedCounter < 2){
                if (higher32 < 4294967295){
                    //increment
                    GC_header newheader =  (((higher32 + 1) << 32) | (lower32mask & header));
                    *headerp = newheader;
                    //logging data
                    GC_header newhigher32 = higher32+1;
                    if (newhigher32 == 1){
                        survives[0]++; 
                        survivesSize[0]+=size; 
                    }else if (newhigher32 == 2){
                        survives[1]++; 
                        survivesSize[1]+=size; 
                    }else if (newhigher32 == 3){
                        survives[2]++; 
                        survivesSize[2]+=size; 
                    }else if (newhigher32 == 4){
                        survives[3]++; 
                        survivesSize[3]+=size; 
                    }else if (newhigher32 == 5){
                        survives[4]++;
                        survivesSize[4]+=size; 
                    }else if (newhigher32 < 10){
                        survives[5]++;
                        survivesSize[5]+=size; 
                    }else if (newhigher32 < 100){
                        survives[6]++;
                        survivesSize[6]+=size; 
                    }else if (newhigher32 < 1000){
                        survives[7]++;
                        survivesSize[7]+=size; 
                    }else if (newhigher32 < 10000){
                        survives[8]++;
                        survivesSize[8]+=size; 
                    }else if (newhigher32 < 100000){
                        survives[9]++;
                        survivesSize[9]+=size; 
                    }else if (newhigher32 < 1000000){
                        survives[10]++;
                        survivesSize[10]+=size; 
                    }else if (newhigher32 < 10000000){
                        survives[11]++;
                        survivesSize[11]+=size; 
                    }else {
                        survives[12]++;
                        survivesSize[12]+=size; 
                    }
                }else{
                    //printf("Heap Profiling hitting max value for gc survived\n");
                    survives[12]++;
                    survivesSize[12]+=size; 
                }
                
            }
        }else if (s->heapProfilingLocation){
            uint32_t sci = higher32; 
            //increment object
            locationObjects[sci]++;
            //add size
            locationSize[sci]+=size;
        }
    }else{
        printf("unexpected header at heap profiling code\n");
    }

    front += size;
    goto updateObject;
    done:
    //end of code to traverse heap 
    
    //Write the time,live bytes,heap size, number of objects
    fwrite(&time_hprofiling,sizeof(uintmax_t),1,s->heapProfilingFile);
    fwrite(&s->heap.oldGenSize,sizeof(size_t),1,s->heapProfilingFile);//same as lastMajorStatistics.bytesLive
    fwrite(&s->heap.size,sizeof(size_t),1,s->heapProfilingFile);
    fwrite(&object_count,sizeof(size_t),1,s->heapProfilingFile);

    //if we are doing lifetimes then write that data after
    if (s->heapProfilingGcSurvived){
        fwrite(&survives[0],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[1],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[2],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[3],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[4],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[5],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[6],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[7],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[8],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[9],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[10],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[11],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survives[12],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[0],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[1],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[2],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[3],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[4],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[5],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[6],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[7],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[8],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[9],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[10],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[11],sizeof(size_t),1,s->heapProfilingFile);
        fwrite(&survivesSize[12],sizeof(size_t),1,s->heapProfilingFile);
    }

    if(s->heapProfilingLocation){
        for(uint32_t i = 0;i < numberNames; numberNames++){
            //write out the #objects for every location
            fwrite(&locationObjects[i],sizeof(size_t),1,s->heapProfilingFile);  
            //write out the objectsumsize for every location
            fwrite(&locationSize[i],sizeof(size_t),1,s->heapProfilingFile);  
            //write out how many characters are in the length string
            const char *res;
            res = getSourceName(s,i);
            size_t len = strlen(res);
            fwrite(&len,sizeof(size_t),1,s->heapProfilingFile);  
            //write out the location string
            fwrite(&res,sizeof(char),len,s->heapProfilingFile);  
            //printf("source index of object we are looking at : %d\n",sourceCodeIndex);
        }
    }

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
