/* Copyright (C) 1999-2005 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void displayGCState (GC_state s, FILE *stream) {
  fprintf (stream,
           "GC state\n");
  fprintf (stream, "\tcurrentThread = "FMTOBJPTR"\n", s->currentThread);
  displayThread (s, (GC_thread)(objptrToPointer (s->currentThread, s->heap.start)), 
                 stream);
  fprintf (stream, "\tgenerational\n");
  displayGenerationalMaps (s, &s->generationalMaps, 
                           stream);
  fprintf (stream, "\theap\n");
  displayHeap (s, &s->heap, 
               stream);
  fprintf (stream,
           "\tlimit = "FMTPTR"\n"
           "\tstackBottom = "FMTPTR"\n"
           "\tstackTop = "FMTPTR"\n",
           (uintptr_t)s->limit,
           (uintptr_t)s->stackBottom,
           (uintptr_t)s->stackTop);
}

size_t sizeofGCStateCurrentStackUsed (GC_state s) {
  return s->stackTop - s->stackBottom;
}

void setGCStateCurrentThreadAndStack (GC_state s) {
  GC_thread thread;
  GC_stack stack;

  thread = getThreadCurrent (s);
  s->exnStack = thread->exnStack;
  stack = getStackCurrent (s);
  s->stackBottom = getStackBottom (s, stack);
  s->stackTop = getStackTop (s, stack);
  s->stackLimit = getStackLimit (s, stack);
  markCard (s, (pointer)stack);
}

void setGCStateCurrentHeap (GC_state s, 
                            size_t oldGenBytesRequested,
                            size_t nurseryBytesRequested) {
  GC_heap h;
  size_t nurserySize;

  if (DEBUG_DETAILED)
    fprintf (stderr, "setGCStateCurrentHeap(%s, %s)\n",
             uintmaxToCommaString(oldGenBytesRequested),
             uintmaxToCommaString(nurseryBytesRequested));
  h = &s->heap;
  assert (isFrontierAligned (s, h->start + h->oldGenSize + oldGenBytesRequested));
  nurserySize = h->size - h->oldGenSize - oldGenBytesRequested;
  s->limitPlusSlop = h->start + h->size;
  s->limit = s->limitPlusSlop - GC_HEAP_LIMIT_SLOP;
  assert (isAligned (nurserySize, POINTER_SIZE));
  if (/* The mutator marks cards. */
      s->mutatorMarksCards
      /* There is enough space in the nursery. */
      and (nurseryBytesRequested
           <= (size_t)(s->limitPlusSlop
                       - alignFrontier (s, (s->limitPlusSlop 
                                            - nurserySize / 2 + 2))))
      /* The nursery is large enough to be worth it. */
      and (((float)(h->size - s->lastMajorStatistics.bytesLive) 
            / (float)nurserySize) 
           <= s->controls.ratios.nursery)
      and /* There is a reason to use generational GC. */
      (
       /* We must use it for debugging pruposes. */
       FORCE_GENERATIONAL
       /* We just did a mark compact, so it will be advantageous to to
        * use it.
        */
       or (s->lastMajorStatistics.kind == GC_MARK_COMPACT)
       /* The live ratio is low enough to make it worthwhile. */
       or ((float)h->size / (float)s->lastMajorStatistics.bytesLive
           <= (h->size < s->sysvals.ram
               ? s->controls.ratios.copyGenerational
               : s->controls.ratios.markCompactGenerational))
       )) {
    s->canMinor = TRUE;
    nurserySize /= 2;
    while (not (isAligned (nurserySize, POINTER_SIZE))) {
      nurserySize -= 2;
    }
    clearCardMap (s);
  } else {
    unless (nurseryBytesRequested
            <= (size_t)(s->limitPlusSlop
                        - alignFrontier (s, s->limitPlusSlop
                                         - nurserySize)))
      die ("Out of memory.  Insufficient space in nursery.");
    s->canMinor = FALSE;
  }
  assert (nurseryBytesRequested
          <= (size_t)(s->limitPlusSlop
                      - alignFrontier (s, s->limitPlusSlop
                                       - nurserySize)));
  s->heap.nursery = alignFrontier (s, s->limitPlusSlop - nurserySize);
  s->frontier = s->heap.nursery;
  assert (nurseryBytesRequested <= (size_t)(s->limitPlusSlop - s->frontier));
  assert (isFrontierAligned (s, s->heap.nursery));
  assert (hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested));
}


bool GC_getAmOriginal (GC_state s) {
  return s->amOriginal;
}
void GC_setAmOriginal (GC_state s, bool b) {
  s->amOriginal = b;
}

void GC_setMessages (GC_state s, bool b) {
  s->controls.messages = b;
}

void GC_setSummary (GC_state s, bool b) {
  s->controls.summary = b;
}

void GC_setRusageMeasureGC (GC_state s, bool b) {
  s->controls.rusageMeasureGC = b;
}

void GC_setHashConsDuringGC (GC_state s, bool b) {
  s->hashConsDuringGC = b;
}

struct rusage* GC_getRusageGCAddr (GC_state s) {
  return &(s->cumulativeStatistics.ru_gc);
}

sigset_t* GC_getSignalsHandledAddr (GC_state s) {
  return &(s->signalsInfo.signalsHandled);
}

sigset_t* GC_getSignalsPendingAddr (GC_state s) {
  return &(s->signalsInfo.signalsPending);
}

void GC_setGCSignalHandled (GC_state s, bool b) {
  s->signalsInfo.gcSignalHandled = b;
}

bool GC_getGCSignalPending (GC_state s) {
  return (s->signalsInfo.gcSignalPending);
}

void GC_setGCSignalPending (GC_state s, bool b) {
  s->signalsInfo.gcSignalPending = b;
}

void GC_setCallFromCHandlerThread (GC_state s, GC_thread t) {
  objptr op = pointerToObjptr ((pointer)t, s->heap.start);
  s->callFromCHandlerThread = op;
}

GC_thread GC_getCurrentThread (GC_state s) {
  pointer p = objptrToPointer (s->currentThread, s->heap.start);
  return (GC_thread)p;
}

GC_thread GC_getSavedThread (GC_state s) {
  pointer p = objptrToPointer (s->savedThread, s->heap.start);
  s->savedThread = BOGUS_OBJPTR;
  return (GC_thread)p;
}

void GC_setSavedThread (GC_state s, GC_thread t) {
  objptr op = pointerToObjptr ((pointer)t, s->heap.start);
  s->savedThread = op;
}

void GC_setSignalHandlerThread (GC_state s, GC_thread t) {
  objptr op = pointerToObjptr ((pointer)t, s->heap.start);
  s->signalHandlerThread = op;
}