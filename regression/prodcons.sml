(* Translated from prodcons.ocaml. *)

functor Z (S: sig
		 structure Primitive:
		    sig
		       structure Stdio:
			  sig
			     val print: string -> unit
			  end
		    end
		 structure MLton:
		    sig
		       structure Itimer:
			  sig
			     datatype t =
				Prof
			      | Real
			      | Virtual

			     val set: t * {value: Time.time,
					   interval: Time.time} -> unit
			  end
		       structure Thread:
			  sig
			     type 'a t

			     val atomicBegin: unit -> unit
			     val atomicEnd: unit -> unit
			     val new: ('a -> unit) -> 'a t
			     val switch: ('a t -> 'b t * 'b) -> 'a
			  end
		       structure Signal:
			  sig
			     type t

			     val alrm: t
			     val handleWith':
				t * (unit Thread.t -> unit Thread.t) -> unit
			     val ignore: t -> unit
			  end
		    end
	      end) =
struct

open S

fun for (start, stop, f) =
   let
      fun loop i =
	 if i > stop
	    then ()
	 else (f i; loop (i + 1))
   in
      loop start
   end

fun print s = () (* Primitive.Stdio.print s *)

structure Queue:
   sig
      type 'a t

      val new: unit -> 'a t
      val enque: 'a t * 'a -> unit
      val deque: 'a t -> 'a option
   end =
   struct
      datatype 'a t = T of {front: 'a list ref, back: 'a list ref}

      fun new () = T {front = ref [], back = ref []}

      fun enque (T {back, ...}, x) = back := x :: !back

      fun deque (T {front, back}) =
	 case !front of
	    [] => (case !back of
		      [] => NONE
		    | l => let val l = rev l
			   in case l of
			      [] => raise Fail "deque"
			    | x :: l => (back := []; front := l; SOME x)
			   end)
	  | x :: l => (front := l; SOME x) 
   end

structure Thread:
   sig
      val exit: unit -> 'a
      val run: unit -> unit
      val spawn: (unit -> unit) -> unit
      val yield: unit -> unit
      structure Mutex:
	 sig
	    type t

	    val new: unit -> t
	    val lock: t * string -> unit
	    val unlock: t -> unit
	 end
      structure Condition:
      	 sig
	    type t
	       
	    val new: unit -> t
	    val signal: t -> unit
	    val wait: t * Mutex.t -> unit
	 end
   end =
   struct
      open MLton
      open Itimer Signal Thread

      val topLevel: unit Thread.t option ref = ref NONE

      local
	 val threads: unit Thread.t Queue.t = Queue.new ()
      in
	 fun ready (t): unit = Queue.enque (threads, t)
	 fun next () =
	    case Queue.deque threads of
	       NONE => (print "switching to toplevel\n"
			; valOf (!topLevel))
	     | SOME t => t
      end
   
      fun 'a exit (): 'a = switch (fn _ => (next (), ()))
      
      fun new (f: unit -> unit): unit Thread.t =
	 Thread.new (fn () => ((f () handle _ => exit ())
			      ; exit ()))
	 
      fun schedule t =
	 (print "scheduling\n"
	  ; ready t
	  ; next ())

      fun yield (): unit = switch (fn t => (schedule t, ()))

      val spawn = ready o new

      fun setItimer t =
	 Itimer.set (Itimer.Real,
		     {value = t,
		      interval = t})

      fun run (): unit =
	 (switch (fn t =>
		  (topLevel := SOME t
		   ; (new (fn () => (handleWith' (alrm, schedule)
				     ; setItimer (Time.fromMilliseconds 20))),
		      ())))
	  ; setItimer Time.zeroTime
	  ; ignore alrm
	  ; topLevel := NONE)
	 
      structure Mutex =
	 struct
	    datatype t = T of {locked: bool ref,
			       waiting: unit Thread.t Queue.t}
	       
	    fun new () =
	       T {locked = ref false,
		  waiting = Queue.new ()}

	    fun lock (T {locked, waiting, ...}, name) =
	       let
		  fun loop () =
		     (print (concat [name, " lock looping\n"])
		      ; Thread.atomicBegin ()
		      ; if !locked
			   then (print "mutex is locked\n"
				 ; switch (fn t =>
					   (Thread.atomicEnd ()
					    ; Queue.enque (waiting, t)
					    ; (next (), ())))
				 ; loop ())
			else (print "mutex is not locked\n"
			      ; locked := true
			      ; Thread.atomicEnd ()))
	       in loop ()
	       end
	    
	    fun safeUnlock (T {locked, waiting, ...}) =
	       (locked := false
		; (case Queue.deque waiting of
		      NONE => ()
		    | SOME t => (print "unlock found waiting thread\n"
				 ; ready t)))

	    fun unlock (m: t) =
	       (print "unlock atomicBegin\n"
		; Thread.atomicBegin ()
		; safeUnlock m
		; Thread.atomicEnd ())
	 end

      structure Condition =
	 struct
	    datatype t = T of {waiting: unit Thread.t Queue.t}

	    fun new () = T {waiting = Queue.new ()}

	    fun wait (T {waiting, ...}, m) =
	       (switch (fn t =>
			(Mutex.safeUnlock m
			 ; print "wait unlocked mutex\n"
			 ; Queue.enque (waiting, t)
			 ; (next (), ())))
		; Mutex.lock (m, "wait"))

	    fun signal (T {waiting, ...}) =
	       case Queue.deque waiting of
		  NONE => ()
		| SOME t => ready t
	 end

   end

structure Mutex = Thread.Mutex
structure Condition = Thread.Condition

val count = ref 0
val data = ref 0
val produced = ref 0
val consumed = ref 0
val m = Mutex.new ()
val c = Condition.new ()

fun producer n =
   for (1, n, fn i =>
	(print (concat ["producer acquiring lock ", Int.toString i, "\n"])
	 ; Mutex.lock (m, "producer")
	 ; print "producer acquired lock\n"
	 ; while !count = 1 do Condition.wait (c, m)
	 ; print "producer passed condition\n"
	 ; data := i
	 ; count := 1
	 ; Condition.signal c
	 ; print "producer releasing lock\n"
	 ; Mutex.unlock m
	 ; print "producer released lock\n"
	 ; produced := !produced + 1))

fun consumer n =
   let val i = ref 0
   in
      while !i <> n do
	 (print (concat ["consumer acquiring lock ", Int.toString (!i), "\n"])
	  ; Mutex.lock (m, "consumer")
	  ; print "consumer acquired lock\n"
	  ; while !count = 0 do Condition.wait (c, m)
	  ; i := !data
	  ; count := 0
	  ; Condition.signal c
	  ; print "consumer releasing lock\n"
	  ; Mutex.unlock m
	  ; print "consumer released lock\n"
	  ; consumed := !consumed + 1)
   end

fun atoi s = case Int.fromString s of SOME num => num | NONE => 0
fun printl [] = TextIO.print "\n" | printl (h::t) = ( TextIO.print h ; printl t )

fun main (name, args) =
   let
      val n = atoi (hd (args @ ["1"]))
      val p = Thread.spawn (fn () => producer n)
      val c = Thread.spawn (fn () => consumer n)
      val _ = Thread.run ()
      val _ = Posix.Process.sleep (Time.fromSeconds 1)
      val _ = printl [Int.toString (!produced),
		      " ",  
		      Int.toString (!consumed)]
   in
      ()
   end

end

structure Z = Z (structure MLton = MLton
		 structure Primitive = Primitive)

val _ = Z.main ( "prodcons", ["1000000"] )
