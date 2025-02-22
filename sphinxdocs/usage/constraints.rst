.. container::
   :name: constraints

-----------
Constraints
-----------

Choosing which constraint to use
--------------------------------

Minion has many constraints which at first glance appear to do almost
identical things. These each have trade-offs, some of which are
difficult to guess in advance. This section will provide some basic
guidance.

One of the major design decisions of Minion’s input language is that it
provides in the input language exactly what it provides internally.
Unlike most other constraint solvers, Minion does not break up
constraints into smaller pieces, introduce new variables or simplify or
manipulate constraints. This provides complete control over how Minion
represents your problem, but also leads to a number of annoyances.

Probably the first thing you will notice it that Minion has neither a
“sum equals” or “weighted sum equals” constraint. This is because the
most efficiently we could implement such a constraint was simply by
gluing together the sumleq and the sumgeq constraints. Minion could
provide a wrapper which generated the two constraints internally, but
this would go against the transparency. Of course if in the future a
more efficient implementation of sumeq was found, it may be added.

The ``watchsumgeq`` and ``watchsumleq`` are varients on the algorithm
used to implement SAT constraints. They are faster than ``sumleq`` and
``sumgeq``, but only work when summing a list of Booleans to a constant.
Further ``watchsumgeq`` performs best when the value being summed to is
small, and ``watchsumleq`` works best when the value being summed to is
close to the size of the input vector.

Minion does not attempt to simplify constraints, so constraints such as
``sumgeq([a,a,a], 3)`` are not simplified to ``sumgeq([a],1)``. This
kind of simplification, done by hand, will often improve models.
Further, and importantly in practice, Minion pre-allocates memory based
on the initial domain size of variables. If these are excessively slack,
this can hurt performance throughout search.

Some constraints in Minion do not work on ``BOUND`` and ``SPARSEBOUND``
variables, in particular ``gacalldiff`` and ``watchelement``. These two
constraints are in general better when they can be used.

All Different, Cardinality and Counting Constraints
---------------------------------------------------

alldiffmatrix
^^^^^^^^^^^^^

``alldiffmatrix(myVec, dim)``

This constraint takes a 2d matrix of size ``dim*dim``.
This constraint impose that the matrix forms a latin-square, that the Elements
of the rows and columns are all different.

It ensures there is a bipartite matching between rows
and columns where the edges in the matching correspond to a pair (row,
column) where the variable in position (row,column) in the matrix may be
assigned to the given value.


Notes
"""""

This constraint adds some extra reasoning in addition to the GAC
Alldifferents on the rows and columns.


alldiff
^^^^^^^^

Forces the input vector of variables to take distinct values.

Suppose the input file had the following vector of variables defined:

DISCRETE myVec[9] {1..9}

Then ``alldiff(myVec)`` enforces all variables in ``myVec`` take different values.

This constraint enforces the same level of consistency as a clique of not equals
constraints.

Related constraints
"""""""""""""""""""

See `gacalldiff <#gacalldiff>`__ for the same constraint that enforces
GAC.



gacalldiff
^^^^^^^^^^

Forces the input vector of variables to take distinct values.

Suppose the input file had the following vector of variables defined:

DISCRETE myVec[9] {1..9}

Then ``gacalldiff(myVec)`` enforces all variables in ``myVec`` take different values.
This constraint enforces generalized arc consistency.


gcc
^^^

The Generalized Cardinality Constraint (GCC) constrains the number of
each value that a set of variables can take.

gcc([primary variables], [values of interest], [capacity variables])

For each value of interest, there must be a capacity variable, which
specifies the number of occurrences of the value in the primary
variables.

This constraint only restricts the number of occurrences of the values
in the value list. There is no restriction on the occurrences of other
values. Therefore the semantics of gcc are identical to a set of
occurrence constraints:

.. code-block::
	occurrence([primary variables], val1, cap1)
	occurrence([primary variables], val2, cap2)
	...

.. _example-2:

Example
"""""""

Suppose the input file had the following vectors of variables defined:

.. code-block::
	
	DISCRETE myVec[9] {1..9}
	BOUND cap[9] {0..2}

The following constraint would restrict the occurrence of values 1..9 in
myVec to be at most 2 each initially, and finally equal to the values of
the cap vector.

.. code-block::

	gcc(myVec, [1,2,3,4,5,6,7,8,9], cap)

.. _notes-5:

Notes
"""""

This constraint enforces a hybrid consistency. It reads the bounds of
the capacity variables, then enforces GAC over the primary variables
only. Then the bounds of the capacity variables are updated using flow
algorithms similar to those proposed by Quimper et al, Improved
Algorithms for the Global Cardinality Constraint (CP 2004).

This constraint provides stronger propagation to the capacity variables
than the gccweak constraint.


gccweak
^^^^^^^^

The Generalized Cardinality Constraint (GCC) (weak variant) implements
the same constraint as ``gcc``, with a weaker (but faster) level
of propagation.

This constraint enforces a hybrid consistency. It reads the bounds of
the capacity variables, then enforces GAC over the primary variables
only. Then the bounds of the capacity variables are updated by counting
values in the domains of the primary variables.

The consistency over the capacity variables is weaker than the gcc
constraint, hence the name gccweak.



occurrence
^^^^^^^^^^

The constraint ``occurrence(vec, elem, count)`` ensures that there are count occurrences of the value elem in the vector
vec.

elem must be a constant, not a variable.

occurrenceleq
^^^^^^^^^^^^^

The constraint ``occurrenceleq(vec, elem, count)`` ensures that there are AT MOST count occurrences of the value elem in
the vector vec.

elem and count must be constants


occurrencegeq
^^^^^^^^^^^^^

The constraint ``occurrencegeq(vec, elem, count)`` ensures that there are AT LEAST count occurrences of the value elem in
the vector vec.

elem and count must be constants


nvalueleq
^^^^^^^^^

The constraint ``nvalueleq(V,x)`` ensures that there are <= x different values assigned to the list of
variables V.

nvaluegeq
^^^^^^^^^

The constraint ``nvaluegeq(V,x)`` ensures that there are >= x different values assigned to the list of
variables V.


Accessing Elements of Arrays
----------------------------


element
^^^^^^^

The constraint ``element(vec, i, e)`` specifies that 
vec[i] = e (treating ``vec`` as a 0-indexed array). This implies that ``i`` is in the range ``[0..len(vec)-1]``.

.. _notes-1:

Notes
"""""

Warning: This constraint is not confluent. Depending on the order the
propagators are called in Minion, the number of search nodes may vary
when using element. To avoid this problem, use watchelement instead.
More details below.

The level of propagation enforced by this constraint is not named,
however it works as follows. For constraint vec[i]=e:

-  After i is assigned, ensures that min(vec[i]) = min(e) and
   max(vec[i]) = max(e).
-  When e is assigned, removes idx from the domain of i whenever e is
   not an element of the domain of vec[idx].
-  When m[idx] is assigned, removes idx from i when m[idx] is not in the
   domain of e.

This level of consistency is designed to avoid the propagator having to
scan through vec, except when e is assigned. It does a quantity of cheap
propagation and may work well in practise on certain problems.

Element is not confluent, which may cause the number of search nodes to
vary depending on the order in which constraints are listed in the input
file, or the order they are called in Minion. For example, the following
input causes Minion to search 41 nodes.

.. code-block::

	MINION 3
	**VARIABLES**
	DISCRETE x[5] {1..5}
	**CONSTRAINTS**
	element([x[0],x[1],x[2]], x[3], x[4]) alldiff([x]) 
	**EOF**

However if the two constraints are swapped over, Minion explores 29
nodes. As a rule of thumb, to get a lower node count, move element
constraints to the end of the list.

Related constraints
"""""""""""""""""""

See `watchelement <#watchelement>`__ for details of a logically identical
constraint that enforces generalised arc consistency.

element_one
^^^^^^^^^^^

The constraint element_one is identical to `element <#element>`__, except that the
vector is indexed from 1 rather than from 0.



watchelement_one
^^^^^^^^^^^^^^^^

This constraint is identical to watchelement, except the vector is
indexed from 1 rather than from 0.

Related constraints
"""""""""""""""""""

See entry `watchelement <#watchelement>`__ for details of watchelement,
which watchelement_one is based on.

watchelement
^^^^^^^^^^^^

The constraint ``watchelement(vec, i, e)`` specifies that ``vec[i] = e``. This implies that
``i`` is in the range ``[0..len(vec)-1]``. Enforces generalised arc consistency.

Related constraints
"""""""""""""""""""

See entry `element <#element>`__ for details of an identical constraint
that enforces a lower level of consistency.

watchelement_undefzero
^^^^^^^^^^^^^^^^^^^^^^

The constraint ``watchelement_undefzero(vec, i, e)``

specifies that, in any solution, either:

- vec[i] = e and i is in the range [0 .. len(v)-1]
- i is outside the index range of vec, and e = 0

This differs from watchelement (and element) which are false if i is outside the
index range of vec.

In general, use watchelement unless you have a special reason to use
this constraint!


Arithmetic Constraints
----------------------

difference
^^^^^^^^^^

The constraint ``difference(x,y,z)`` ensures that z=|y-x|. This constraint achieves bounds consistency


eq
^^^^

``eq(x,y)`` ensures that ``x=y``. This constraint implements bounds consistency.

Related constraints
"""""""""""""""""""

`minuseq <#minuseq-1>`__

minuseq
^^^^^^^

``minuseq(x,y)`` ensures that ``x=-y``. The constraint implements bounds consistency.

Related constraints
"""""""""""""""""""

`eq <#eq-1>`__

diseq
^^^^^

``diseq(x,y)`` ensures that ``x`` is not equal ``y``. Achieves arc consistency.

ineq
^^^^

The constraint ``ineq(x, y, k)`` ensures that ``x <= y + k`` in any solution.
Minion has no strict inequality (<) constraints. However x < y can be
achieved by ``ineq(x, y, -1)``

watchless
^^^^^^^^^

The constraint watchless(x,y) ensures that x is less than y.

Related constraints
"""""""""""""""""""

   `ineq <#ineq>`__


abs
^^^

The constraint ``abs(x,y)`` makes sure that ``x=|y|``, i.e. x is the absolute value of y.


max
^^^^^^^^

The constraint ``max(vec, x)`` ensures that ``x`` is equal to the maximum value of any variable in ``vec``.

Related constraints
"""""""""""""""""""

See `min <#min>`__ for the opposite constraint.

min
^^^^^^^^

The constraint ``min(vec, x)`` ensures that ``x`` is equal to the minimum value of any variable in ``vec``.

Related constraints
"""""""""""""""""""

See `max <#max>`__ for the opposite constraint.




div
^^^^^^^^

The constraint ``div(x,y,z)``

ensures that floor(x/y)=z.

For example:

- 10/3 = 3
- (-10)/3 = -4
- 10/(-3) = -4
- (-10)/(-3) = 3

div and mod satisfy together the condition that:

y*(x/y) + x % y = x

The constraint is always false when y = 0



div_undefzero
^^^^^^^^^^^^^

The constraint ``div_undefzero(x,y,z)``

is the same as ``div`` (it ensures that floor(x/y)=z) except the constraint
is always true when y = 0, instead of false.

This constraint exists for certain special requirements. In general, if
you are unsure what constraint to use, then what you want is a plain div
constraint!


modulo
^^^^^^

The constraint ``modulo(x,y,z)`` ensures that x%y=z i.e. z is the remainder of dividing x by y. For
negative values, we ensure that:

y(x/y) + x%y = x

To be fully concrete, here are some examples:

- 3 % 5 = 3
- -3 % 5 = 2
- 3 % -5 = -2
- -3 % -5 = -3


mod_undefzero
^^^^^^^^^^^^^

The constraint ``mod_undefzero(x,y,z)`` is the same as ``modulo`` except the constraint is always true when y = 0,
instead of false.

This constraint exists for certain special requirements. In general, if
you are unsure what constraint to use, then what you want is a plain mod
constraint!


product
^^^^^^^^

The constraint ``product(x,y,z)`` ensures that ``z=x*y`` in any solution.

This constraint can be used for (and, in fact, has a specialised
implementation for) achieving boolean AND, i.e. x & y=z can be modelled
as ``product(x,y,z)``

The general constraint achieves bounds generalised arc consistency for
positive numbers.

pow
^^^^^^^^

The constraint ``pow(x,y,z)``

ensures that x^y=z.

This constraint is only available for positive domains x, y and z.


sumleq
^^^^^^^^

The constraint ``sumleq(vec, c)`` ensures that sum(vec) <= c.

sumgeq
^^^^^^^^

The constraint ``sumgeq(vec, c)``


weightedsumleq
^^^^^^^^^^^^^^

The constraint ``weightedsumleq(constantVec, varVec, total)``

ensures that constantVec.varVec <= total, where constantVec.varVec is
the scalar dot product of constantVec and varVec.


weightedsumgeq
^^^^^^^^^^^^^^

The constraint ``weightedsumgeq(constantVec, varVec, total)`` ensures that constantVec.varVec >= total, where constantVec.varVec is
the scalar dot product of constantVec and varVec.



Table constraints
-----------------


table
^^^^^^^^

An extensional constraint that enforces GAC. The constraint is specified
via a list of tuples.

The variables used in the constraint have to be BOOL or DISCRETE
variables. Other types are not supported.

To specify a constraint over 3 variables that allows assignments
(0,0,0), (1,0,0), (0,1,0) or (0,0,1) do the following.

1) Add a tuplelist to the *``*TUPLELIST**`` section, e.g.:


.. code-block::

	**TUPLELIST**
	myext 4 3
	0 0 0
	1 0 0
	0 1 0
	0 0 1

N.B. the number 4 is the number of tuples in the constraint, the number
3 is the -arity.

2) Add a table constraint to the **CONSTRAINTS** section, e.g.:

.. code-block::

	**CONSTRAINTS** 
	table(myvec, myext)

and now the variables of myvec will satisfy the constraint myext.

The constraints extension can also be specified in the constraint
definition, e.g.:

``table(myvec, {<0,0,0>,<1,0,0>,<0,1,0>,<0,0,1>})``

negativetable
^^^^^^^^^^^^^

An extensional constraint that enforces GAC. The constraint is specified
via a list of disallowed tuples.

See ``table`` for how to specify a table constraint in minion input. The only
difference for negativetable is that the specified tuples are
disallowed.



gacschema
^^^^^^^^^

An extensional constraint that enforces GAC. The constraint is specified
via a list of tuples.

The format, and usage of gacschema, is identical to the 'table'
constraint. It is difficult to predict which out of 'table' and
'gacschema' will be faster for any particular problem.



haggisgac-stable
^^^^^^^^^^^^^^^^

An extensional constraint that enforces GAC. haggisgac-stable is a
variant of haggisgac which uses less memory in some cases, and can also
be faster (or slower). The input is identical to haggisgac.

Related constraints
"""""""""""""""""""

`haggisgac <#haggisgac>`__

haggisgac
^^^^^^^^^

An extensional constraint that enforces GAC. This constraint make uses
of 'short tuples', which allow some values to be marked as don't care.
When this allows the set of tuples to be reduced in size, this leads to
performance gains.

The variables used in the constraint have to be BOOL or DISCRETE
variables. Other types are not supported.

.. _example-3:

Example
"""""""

Consider the constraint 'min([x1,x2,x3],x4)'' on Booleans variables
x1,x2,x3,x4.

Represented as a TUPLELIST for a table or gacschema constraint, this
would look like:

.. code-block::

	**TUPLELIST** mycon 8 4
	0 0 0 0
	0 0 1 0
	0 1 0 0
	0 1 1 0
	1 0 0 0
	1 0 1 0
	1 1 0 0
	1 1 1 1

Short tuples give us a way of shrinking this list. Short tuples consist
of pairs (x,y), where x is a varible position, and y is a value for that
variable. For example:

[(0,0),(3,0)]

Represents 'If the variable at index 0 is 0, and the variable at index 3
is 0, then the constraint is true'.

This allows us to represent our constraint as follows:

.. code-block::

	**SHORTTUPLELIST**
	mycon 4
	[(0,0),(3,0)]
	[(1,0),(3,0)]
	[(2,0),(3,0)]
	[(0,1),(1,1),(2,1),(3,1)]

Note that some tuples are double-represented here. The first 3 short
tuples all allow the assignment ``0 0 0 0``. This is fine. The important
thing for efficency is to try to give a small list of short tuples.

We use this tuple by writing ``haggisgac([x1,x2,x3,x4], mycon)`` and now the variables [x1,x2,x3,x4] will satisfy the constraint mycon.

mddc
^^^^

MDDC (mddc) is an implementation of MDDC(sp) by Cheng and Yap. It
enforces GAC on a constraint using a multi-valued decision diagram
(MDD).

The MDD required for the propagator is constructed from a set of
satisfying tuples. The constraint has the same syntax as 'table' and can
function as a drop-in replacement.

For examples on how to call it, see the help for 'table'. Substitute
'mddc' for 'table'. This constraint enforces generalized arc consistency.

negativemddc
^^^^^^^^^^^^

Negative MDDC (negativemddc) is an implementation of MDDC(sp) by Cheng
and Yap. It enforces GAC on a constraint using a multi-valued decision
diagram (MDD).

The MDD required for the propagator is constructed from a set of
unsatisfying (negative) tuples. The constraint has the same syntax as
'negativetable' and can function as a drop-in replacement.
This constraint enforces generalized arc consistency.


lighttable
^^^^^^^^^^

An extensional constraint that enforces GAC. The constraint is specified
via a list of tuples. lighttable is a variant of the table constraint
that is stateless and potentially faster for small constraints.

For full documentation, see the help for the table constraint.

shortctuplestr2 
^^^^^^^^^^^^^^^

This constraint extends the ShortSTR2 algorithm to support short
c-tuples (that is, short tuples which contain can contain more than one
domain value per constraint).

.. _example-7:

Example
"""""""

Input format is similar to that used by other short tuple constraints,
such as haggisgac or shortstr2. Refer to the haggisgac and
shorttuplelist pages for more information.

The important change is that more than one literal may be given for each
variable. Variables which are not mentioned are assumed to be allowed to
take any value

Example:

.. code-block::

	**SHORTTUPLELIST**
	mycon 4
	[(0,0),(0,1),(3,0)]
	[(1,0),(1,2),(3,0)]
	[(2,0),(3,0),(3,1)]
	[(0,1),(1,1),(2,1),(3,1)]

	**CONSTRAINTS**
	shortctuplestr2([x1,x2,x3,x4], mycon)

This constraint enforces generalized arc consistency.

shortstr2
^^^^^^^^^

ShortSTR2 is the algorithm described in the IJCAI 2013 paper by
Jefferson and Nightingale. It is an extension of STR2+ by Christophe
Lecoutre, adapted f

Input format is exactly the same as haggisgac. Refer to the haggisgac
and shorttuplelist pages for more information.

This constraint enforces generalized arc consistency.

str2plus
^^^^^^^^

str2plus is an implementation of the STR2+ algorithm by Christophe
Lecoutre.

str2plus is invoked in the same way as other table constraints, such
as table and mddc.

This constraint enforces generalized arc consistency.


Lexicographic Ordering
----------------------

lexleq[rv]
^^^^^^^^^^

The constraint ``lexleq[rv](vec0, vec1)`` takes two vectors vec0 and vec1 of the same length and ensures that
vec0 is lexicographically less than or equal to vec1 in any solution.

This constraint achieves GAC even when some variables are repeated in
vec0 and vec1. However, the extra propagation this achieves is rarely
worth the extra work.

Related constraints
"""""""""""""""""""

See `lexleq[quick] <>`__ for a much faster logically identical
constraint, with lower propagation.

lexless
^^^^^^^

The constraint ``lexless(vec0, vec1)`` takes two vectors vec0 and vec1 of the same length and ensures that vec0
is lexicographically less than vec1 in any solution. This constraint maintains GAC.

Related constraints
"""""""""""""""""""

See `lexleq <#lexleq>`__ for a similar constraint with non-strict
lexicographic inequality.

lexleq
^^^^^^

The constraint ``lexleq(vec0, vec1)`` takes two vectors vec0 and vec1 of the same length and ensures that vec0
is lexicographically less than or equal to vec1 in any solution. This constraints achieves GAC.

Related constraints
"""""""""""""""""""

See `lexless <#lexless>`__ for a similar constraint with strict
lexicographic inequality.



litsumgeq
^^^^^^^^^

The constraint litsumgeq(vec1, vec2, c) ensures that there exists at
least c distinct indices i such that vec1[i] = vec2[i].

.. _notes-20:

Notes
"""""

A SAT clause {x,y,z} can be created using:

   litsumgeq([x,y,z],[1,1,1],1)

Note also that this constraint is more efficient for smaller values of
c. For large values consider using watchsumleq.

This constraint is not reifiable.

Related constraints
"""""""""""""""""""

   `watchsumleq <#watchsumleq>`__ `watchsumgeq <#watchsumgeq>`__


Constraints which operate on other Constraints
----------------------------------------------

The input language of Minion is mostly "flat". There are a number of constraints which do accept
other constraints as arguments, which are given in this section.

watched-and
^^^^^^^^^^^

The constraint ``watched-and({C1,...,Cn})`` ensures that the constraints C1,...,Cn are all true.

Conjunctions of constraints may seem pointless, bearing in mind that a CSP is simply a conjunction of
constraints already! ``watched-and`` is provided to use with other nested constraint, for example in a
reification: ``reify(watched-and({...}),r)``

Related constraints
"""""""""""""""""""

   `watched-or <#watched-or>`__


watched-or
^^^^^^^^^^

The constraint ``watched-or({C1,...,Cn})`` ensures that at least one of the constraints C1,...,Cn is true.

Related constraints
"""""""""""""""""""

   `watched-and <#watched-and>`__

   
reify and reifyimply
^^^^^^^^^^^^^^^^^^^^

Reification is provided in two forms: reify and reifyimply.

   ``reify(constraint, r)`` where r is a 0/1 var

ensures that r is set to 1 if and only if constraint is satisfied. That
is, if r is 0 the constraint must NOT be satisfied; and if r is 1 it
must be satisfied as normal. Conversely, if the constraint is satisfied
then r must be 1, and if not then r must be 0.

   ``reifyimply(constraint, r)``

only checks that if r is set to 1 then constraint must be satisfied. If
r is not 1, constraint may be either satisfied or unsatisfied.
Furthermore r is never set by propagation, only by search; that is,
satisfaction of constraint does not affect the value of r.

All constraints are reifyable and reifyimplyable, except where explictly stated.


Matrix Constraints
------------------

watchsumgeq
^^^^^^^^^^^

The constraint ``watchsumgeq(vec, c)`` ensures that sum(vec) >= c, for a list of 0/1 variables ``vec``.

For this constraint, small values of c are more efficient. This is equivalent to ``litsumgeq(vec, [1,...,1], c), but faster.


Related constraints
"""""""""""""""""""

   `watchsumleq <#watchsumleq>`__ `litsumgeq <#litsumgeq>`__

watchsumleq
^^^^^^^^^^^

The constraint ``watchsumleq(vec, c)`` ensures that sum(vec) <= c, for a list of 0/1 variables ``vec``.


Equivalent to litsumgeq([vec1,...,vecn], [0,...,0], n-c) but faster for large values of ``c``.

Related constraints
"""""""""""""""""""

   `watchsumgeq <#watchsumgeq>`__ `litsumgeq <#litsumgeq>`__

hamming
^^^^^^^

The constraint ``hamming(X,Y,c)`` ensures that the hamming distance between X and Y is at least c. That
is, that the size of the set {i \| X[i] != y[i]} is greater than or
equal to c.

watchvecneq
^^^^^^^^^^^

The constraint watchvecneq(A, B)

ensures that A and B are not the same vector, i.e., there exists some
index i such that A[i] != B[i].


Unary constraints
-----------------


w-inrange
^^^^^^^^^

The constraint w-inrange(x, [a,b]) ensures that a <= x <= b.

w-inset
^^^^^^^^

The constraint w-inset(x, [a1,...,an]) ensures that x belongs to the set
{a1,..,an}.

w-literal
^^^^^^^^^

The constraint w-literal(x, a) ensures that x=a.

w-notinrange
^^^^^^^^^^^^

The constraint w-notinrange(x, [a,b]) ensures that x < a or b < x.

w-notinset
^^^^^^^^^^

The constraint w-notinset(x, [a1,...,an]) ensures that x does not belong
to the set {a1,..,an}.



w-notliteral
^^^^^^^^^^^^

The constraint w-notliteral(x, a) ensures that x =/= a.

w-inintervalset
^^^^^^^^^^^^^^^

The constraint w-inintervalset(x, [a1,a2, b1,b2, ... ]) ensures that the
value of x belongs to one of the intervals {a1,...,a2}, {b1,...,b2} etc.
The list of intervals must be given in numerical order.
