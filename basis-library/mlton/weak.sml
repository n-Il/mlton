structure MLtonWeak =
   struct
      structure Weak = Primitive.MLton.Weak
	 
      type 'a t = 'a Weak.t

      val new = Weak.new

      fun get (w: 'a t): 'a option =
	 let
	    (* Need to do the canGet after the get.  If you did the canGet first,
	     * there could be a GC that invalidates the pointer between the
	     * canGet and the get.
	     *)
	    val x = Weak.get w
	 in
	    if Weak.canGet w
	       then SOME x
	    else NONE
	 end
   end
