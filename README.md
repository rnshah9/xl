ELIOT - Extensible Language for the Internet of Things
======================================================

ELIOT is a very flexible language designed to make sensors or actuators
smarter and more efficient. It is a derivative of XLR, optimized for
very small devices, and easily embeddable in your application.





In ELIOT, you don't define functions or variables. You define "rewrites"
of the code. This is done with the rewrite operator `->` which reads
as "transforms into".

For example, `X -> 0` means that you should transform `X` into `0`.
Below is the definition of a factorial in ELIOT:

```
0! -> 1
N! -> N * (N-1)!
```

Internally, ELIOT parses source input into a parse tree containing only 8
node types.

Four node types represent leafs in the parse tree:

* Integer nodes represents integral numbers such as `123`.
* Real nodes represent floating-point numbers such as `3.1415`.
* Text nodes represent textual data such as `"ABC"`
* Name nodes represent names and symbols such as `ABC` or `<<`

Four other node types represent inner nodes in the parse tree:

* Infix nodes represent notations such as `A+B` or `A and B`
* Prefix nodes represent notations such as `+A` or `sin A`
* Postfix nodes represent notations such as `3%` or `5 km`
* Block nodes represent notations such as `(A)`, `[A]` or `{A}`.

Block nodes are used to represent indentation in the source code.
Infix nodes are used to represent line breaks in the source code.

ELIOT is used as the basis for Tao, a functional, reactive, dynamic
document description language for 3D animations.
See http://www.taodyne.com for more information on Tao.
In action: http://www.youtube.com/embed/Fvi29XAo4SI.


The xl-symbols branch
=====================

This branch is a major refactoring of the compiler with the following
objectives:

* Store the symbol tables using ELIOT data structures.
* Generate better code, notably using type inference where applicable
* Generally speaking, cleanup all the crud that accumulated over time

The compiler has already been significantly reduced compared to
'master'. It currently runs at less than 25000 lines of code. A minor
issue is that it does not work at all. Yet.

The symbol table uses a data structure that can almost be evaluated to
what it represents. Specifically:

* Scopes are represented by `Prefix` nodes. If scope `A` is inside
  scope `B`, the representation is `B A`, e.g. `(X->1) (Y->2; Z->3)`
* Declarations are represented by `Infix` nodes, where a `\n` infix
  represents a declaration, its left node being something like `A->B`,
  and its right node being an `;` infix, where the left and right node
  are hash-balanced children declarations.

