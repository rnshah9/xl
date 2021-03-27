#ifndef TREE_H
#define TREE_H
// *****************************************************************************
// tree.h                                                             XL project
// *****************************************************************************
//
// File description:
//
//     Basic representation of parse tree.
//
//     See the big comment at the top of parser.h for details about
//     the basics of XL  tree representation
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2010-2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2005,2009-2012,2014-2021, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2011, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "base.h"
#include "gc.h"
#include "info.h"
#include "recorder/recorder.h"

#include <map>
#include <vector>
#include <cassert>
#include <iostream>
#include <cctype>


XL_BEGIN

// ============================================================================
//
//    The types being defined or used to define XL trees
//
// ============================================================================

struct Tree;                                    // Base tree
struct Natural;                                 // Natural: 0, 3, 8
struct Real;                                    // Real: 3.2, 1.6e4
struct Text;                                    // Text: "ABC"
struct Name;                                    // Name / symbol: ABC, ++-
struct Block;                                   // Block: (A), {A}
struct Prefix;                                  // Prefix: sin X
struct Postfix;                                 // Postfix: 3!
struct Infix;                                   // Infix: A+B, newline
struct Info;                                    // Information in trees
struct Context;                                 // Execution context



// ============================================================================
//
//    Pointer and structure types
//
// ============================================================================

typedef GCPtr<Tree>                     Tree_p;
typedef GCPtr<Natural, longlong>        Natural_p;
typedef GCPtr<Real, double>             Real_p;
typedef GCPtr<Text, text>               Text_p;
typedef GCPtr<Name>                     Name_p;
typedef GCPtr<Block>                    Block_p;
typedef GCPtr<Prefix>                   Prefix_p;
typedef GCPtr<Postfix>                  Postfix_p;
typedef GCPtr<Infix>                    Infix_p;
struct Scope;

typedef uintptr_t TreePosition;                 // Position in source files
typedef std::vector<Tree_p> TreeList;           // A list of trees
typedef Tree *(*eval_fn) (Scope *, Tree *);     // Compiled evaluation code



// ============================================================================
//
//    The Tree class
//
// ============================================================================

enum kind
// ----------------------------------------------------------------------------
//   The kinds of tree that compose an XL parse tree
// ----------------------------------------------------------------------------
{
    NATURAL, REAL, TEXT, NAME,                  // Leaf nodes
    BLOCK, PREFIX, POSTFIX, INFIX,              // Non-leaf nodes

    KIND_FIRST          = NATURAL,
    KIND_LAST           = INFIX,
    KIND_LEAF_FIRST     = NATURAL,
    KIND_LEAF_LAST      = NAME,
    KIND_NLEAF_FIRST    = BLOCK,
    KIND_NLEAF_LAST     = INFIX
};


// Must be out of the enum to avoid warnings about unhandled enum cases
const int KIND_COUNT = KIND_LAST+1;


struct Tree
// ----------------------------------------------------------------------------
//   The base class for all XL trees
// ----------------------------------------------------------------------------
{
    enum { KINDBITS = 3, SIGNBIT = 3, SRCBIT = 4, POSBITS = 5, KINDMASK=7 };
    enum { UNKNOWN_POSITION = ~0UL, COMMAND_LINE=~1UL, BUILTIN=~2UL };
    typedef Tree        self_t;
    typedef Tree *      value_t;
    const uintptr_t     FROM_SOURCE = 1 << SRCBIT;

    // Constructor and destructor
    Tree (kind k, TreePosition pos = NOWHERE):
        tag((pos<<POSBITS) | k), info(), code() {}
    Tree(kind k, Tree *from):
        tag(from->tag), info(), code()
    {
        assert(k == Kind()); (void) k;
    }
    ~Tree();

    // Perform recursive actions on a tree
    template<typename Action>
    typename Action::value_type Do(Action *a);
    template<typename Action>
    typename Action::value_type Do(Action &a) { return Do<Action>(&a); }

    // Attributes
    kind                Kind()                { return kind(tag & KINDMASK); }
    TreePosition        Position()            { return (long) tag>>POSBITS; }
    bool                IsValid()             { return IsNull(this); }
    bool                IsLeaf()              { return Kind() <= NAME; }
    bool                IsConstant()          { return Kind() <= TEXT; }
    void                SetPosition(TreePosition pos, bool recurse = true);
    bool                IsFromSource()        { return tag & FROM_SOURCE; }
    void                MarkSourceTree()      { tag |= FROM_SOURCE; }

    // Safe cast to an appropriate subclass
    template<class T>
    T *                 As(Scope *context = nullptr);
    Natural *           AsNatural();
    Real *              AsReal();
    Text *              AsText();
    Name *              AsName();
    Block *             AsBlock();
    Infix *             AsInfix();
    Prefix *            AsPrefix();
    Postfix *           AsPostfix();
    Tree *              AsTree();


    // Info management
    template<class I>    typename I::data_t  Get() const;
    template<class I>    void                Set(typename I::data_t data);
    template<class I>    I*                  GetInfo() const;
    template<class I>    void                SetInfo(I *);
    template<class I>    bool                Exists() const;
    template<class I>    bool                Purge();
    template<class I>    I*                  Remove();
    template<class I>    I*                  Remove(I *);

    // Conversion to text
                        operator text();

    struct Iterator
    // ------------------------------------------------------------------------
    //   Iterate on a tree without recursion
    // ------------------------------------------------------------------------
    {
        Iterator(Tree *tree): tree(tree), current(tree), next() {}
        Tree *operator()();

    private:
        Tree_p              tree;
        Tree_p              current;
        std::vector<Tree_p> next;
    };


public:
    static int          Compare(Tree *t1, Tree *t2, bool recurse = true);
    static bool         Equal(Tree *t1, Tree *t2, bool recurse = true);

public:
    uintptr_t           tag;                            // Position + kind
    Atomic<Info *>      info;                           // Information for tree
    void *              code;                           // Pointer to code

    static TreePosition NOWHERE;
    GARBAGE_COLLECT(Tree);
    static kstring      KindName[KIND_COUNT];

private:
    Tree (const Tree &);
};



// ============================================================================
//
//   Leaf nodes (natural, real, name, text)
//
// ============================================================================

struct Natural : Tree
// ----------------------------------------------------------------------------
//   Natural constants
// ----------------------------------------------------------------------------
{
    static const kind KIND = NATURAL;
    typedef Natural  self_t;
    typedef ulonglong value_t;

    Natural(value_t i = 0, TreePosition pos = NOWHERE):
        Tree(NATURAL, pos), value(i) {}
    Natural(Natural *i): Tree(NATURAL, i), value(i->value) {}
    value_t  value;
    operator value_t()                  { return value; }

    static Natural *Signed(longlong x, TreePosition pos = NOWHERE)
    {
        Natural *result = new Natural((ulonglong) x, pos);
        result->tag |= (1 << SIGNBIT);
        return result;
    }

    bool        IsSigned()              { return tag & (1 << SIGNBIT); }
    Natural *   MakeSigned();
    Natural *   MakeUnsigned();

    GARBAGE_COLLECT(Natural);
};


struct Real : Tree
// ----------------------------------------------------------------------------
//   Real numbers
// ----------------------------------------------------------------------------
{
    static const kind KIND = REAL;
    typedef Real   self_t;
    typedef double value_t;

    Real(value_t d = 0.0, TreePosition pos = NOWHERE):
        Tree(REAL, pos), value(d) {}
    Real(Real *r): Tree(REAL, r), value(r->value) {}
    value_t  value;
    operator value_t()           { return value; }
    GARBAGE_COLLECT(Real);
};


struct Text : Tree
// ----------------------------------------------------------------------------
//   Text, e.g. "Hello World"
// ----------------------------------------------------------------------------
{
    static const kind KIND = TEXT;
    typedef Text self_t;
    typedef text value_t;

    Text(value_t t, text open="\"", text close="\"", TreePosition pos=NOWHERE):
        Tree(TEXT, pos), value(t), opening(open), closing(close) {}
    Text(value_t t, TreePosition pos):
        Tree(TEXT, pos), value(t), opening(textQuote), closing(textQuote) {}
    Text(Text *t):
        Tree(TEXT, t),
        value(t->value), opening(t->opening), closing(t->closing) {}
    value_t             value;
    text                opening, closing;
    static text         textQuote, charQuote;
    operator value_t()  { return value; }
    bool IsCharacter()  { return (opening == "'" &&
                                  closing == "'" &&
                                  value.length() == 1); }
    bool IsBinary()     { return opening == "" && closing == ""; }

    static Text *Character(char c)
    {
        return new Text(text(1, c), "'", "'");
    }

    GARBAGE_COLLECT(Text);
};


struct Name : Tree
// ----------------------------------------------------------------------------
//   A node representing a name or symbol
// ----------------------------------------------------------------------------
{
    static const kind KIND = NAME;
    typedef Name self_t;
    typedef text value_t;

    Name(value_t n, value_t s, TreePosition pos = NOWHERE):
        Tree(NAME, pos), value(n), spelling(s) { }
    Name(value_t n, TreePosition pos = NOWHERE):
        Tree(NAME, pos), value(n), spelling(n) { }
    Name(Name *n):
        Tree(NAME, n), value(n->value), spelling(n->spelling) { }
    bool        IsEmpty()       { return value.length() == 0; }
    bool        IsOperator()    { return !IsEmpty() && !isalpha(value[0]); }
    bool        IsName()        { return !IsEmpty() && isalpha(value[0]); }
    bool        IsBoolean()     { return value=="true" || value=="false"; }
    value_t     value;
    value_t     spelling;
    operator    value_t()       { return value; }
    GARBAGE_COLLECT(Name);
};



// ============================================================================
//
//   Structured types: Block, Prefix, Infix
//
// ============================================================================

struct Block : Tree
// ----------------------------------------------------------------------------
//   A block, such as (X), {X}, [X] or indented block
// ----------------------------------------------------------------------------
{
    static const kind KIND = BLOCK;
    typedef Block       self_t;
    typedef Block *     value_t;

    Block(Tree *c, text open, text close, TreePosition pos = NOWHERE):
        Tree(BLOCK, pos), child(c), opening(open), closing(close) {}
    Block(Block *b, Tree *ch):
        Tree(BLOCK, b),
        child(ch), opening(b->opening), closing(b->closing) {}
    bool IsIndent()     { return opening == indent && closing == unindent; }
    bool IsParentheses(){ return opening == "(" && closing == ")"; }
    bool IsBraces()     { return opening == "{" && closing == "}"; }
    bool IsSquare()     { return opening == "[" && closing == "]"; }
    bool IsGroup()      { return IsIndent() || IsParentheses() || IsBraces(); }
    Tree *IsMetaBox()
    {
        // Check if block looks like [[A]], if so return A otherwise nullptr
        if (IsSquare())
            if (Block *block = child->AsBlock())
                if (block->IsSquare())
                    return block->child;
        return nullptr;
    }
    static Block *MetaBox(Tree *child)
    {
        TreePosition pos = child->Position();
        return new Block(new Block(child, "[", "]", pos), "[", "]", pos);
    }
    Tree_p              child;
    text                opening, closing;
    static text         indent, unindent;
    GARBAGE_COLLECT(Block);
};


struct Prefix : Tree
// ----------------------------------------------------------------------------
//   A prefix operator, e.g. sin X, +3
// ----------------------------------------------------------------------------
{
    static const kind KIND = PREFIX;
    typedef Prefix      self_t;
    typedef Prefix *    value_t;

    Prefix(Tree *l, Tree *r, TreePosition pos = NOWHERE):
        Tree(PREFIX, pos), left(l), right(r) {}
    Prefix(Prefix *p, Tree *l, Tree *r):
        Tree(PREFIX, p), left(l), right(r) {}
    Tree_p               left;
    Tree_p               right;
    GARBAGE_COLLECT(Prefix);
};


struct Postfix : Tree
// ----------------------------------------------------------------------------
//   A postfix operator, e.g. 3!
// ----------------------------------------------------------------------------
{
    static const kind KIND = POSTFIX;
    typedef Postfix     self_t;
    typedef Postfix *   value_t;

    Postfix(Tree *l, Tree *r, TreePosition pos = NOWHERE):
        Tree(POSTFIX, pos), left(l), right(r) {}
    Postfix(Postfix *p, Tree *l, Tree *r):
        Tree(POSTFIX, p), left(l), right(r) {}
    Tree_p              left;
    Tree_p              right;
    GARBAGE_COLLECT(Postfix);
};


struct Infix : Tree
// ----------------------------------------------------------------------------
//   Infix operators, e.g. A+B, A and B, A,B,C,D,E
// ----------------------------------------------------------------------------
{
    static const kind KIND = INFIX;
    typedef Infix       self_t;
    typedef Infix *     value_t;

    Infix(text n, Tree *l, Tree *r, TreePosition pos = NOWHERE):
        Tree(INFIX, pos), left(l), right(r), name(n), spelling(n) { }
    Infix(text n, text s, Tree *l, Tree *r, TreePosition pos = NOWHERE):
        Tree(INFIX, pos), left(l), right(r), name(n), spelling(s) { }
    Infix(Infix *i, Tree *l, Tree *r):
        Tree(INFIX, i),left(l),right(r), name(i->name),spelling(i->spelling) { }
    Infix *             LastStatement(text sep1 = ";", text sep2 = "\n");
    Tree_p              left;
    Tree_p              right;
    text                name;
    text                spelling;
    GARBAGE_COLLECT(Infix);
};



// ============================================================================
//
//    Safe casts
//
// ============================================================================

template <class T>
bool IsNotNull(T *ptr)
// ----------------------------------------------------------------------------
//   Workaround overly capable compilers that "know" this can't be nullptr
// ----------------------------------------------------------------------------
{
    return ptr != 0;
}


template<class T>
inline T * Tree::As(Scope *)
// ----------------------------------------------------------------------------
//   Return a pointer to the given class
// ----------------------------------------------------------------------------
//   By default, we only check the kind
{
    if (IsNotNull(this) && Kind() == T::KIND)
        return (T *) this;
    return nullptr;
}

template<>
inline Tree *Tree::As<Tree>(Scope *)
// ----------------------------------------------------------------------------
//    Special case for Tree
// ----------------------------------------------------------------------------
{
    return this;
}


inline Natural *Tree::AsNatural()       { return As<Natural>(); }
inline Real    *Tree::AsReal()          { return As<Real>(); }
inline Text    *Tree::AsText()          { return As<Text>(); }
inline Name    *Tree::AsName()          { return As<Name>(); }
inline Block   *Tree::AsBlock()         { return As<Block>(); }
inline Prefix  *Tree::AsPrefix()        { return As<Prefix>(); }
inline Postfix *Tree::AsPostfix()       { return As<Postfix>(); }
inline Infix   *Tree::AsInfix()         { return As<Infix>(); }
inline Tree    *Tree::AsTree()          { return As<Tree>(); }



// ============================================================================
//
//   Template members for recursive operations on trees
//
// ============================================================================

template<typename Action>
typename Action::value_type Tree::Do(Action *action)
// ----------------------------------------------------------------------------
//   Perform an action on the tree
// ----------------------------------------------------------------------------
{
    switch(Kind())
    {
    case NATURAL:       return action->Do((Natural *) this);
    case REAL:          return action->Do((Real *) this);
    case TEXT:          return action->Do((Text *) this);
    case NAME:          return action->Do((Name *) this);
    case BLOCK:         return action->Do((Block *) this);
    case PREFIX:        return action->Do((Prefix *) this);
    case POSTFIX:       return action->Do((Postfix *) this);
    case INFIX:         return action->Do((Infix *) this);
    default:            assert(!"Unexpected tree kind");
    }
    return typename Action::value_type();
}



// ============================================================================
//
//   Sequence iterator - Iterate without deep recursion
//
// ============================================================================

inline bool Tree::Equal(Tree *t1, Tree *t2, bool recurse)
// ----------------------------------------------------------------------------
//   Compare for equality
// ----------------------------------------------------------------------------
{
    return Compare(t1, t2, recurse) == 0;
}


inline Infix *Infix::LastStatement(text sep1, text sep2)
// ----------------------------------------------------------------------------
//   Return the last statement following a given infix
// ----------------------------------------------------------------------------
{
    Infix *last = this;
    while (Infix *next = last->right->AsInfix())
    {
        if (next->name == sep1 || next->name == sep2)
            last = next;
        else
            break;
    }
    return last;
}



// ============================================================================
//
//   Conversion between signed and unsigned integers
//
// ============================================================================

inline Natural *Natural::MakeSigned()
// ----------------------------------------------------------------------------
//   If the number is not signed, make a new signed version of it
// ----------------------------------------------------------------------------
//   We need to create a copy so that the original number is immutable, as
//   it may have been cached e.g. during evaluation.
{
    if (IsSigned())
        return this;
    Natural *snat = new Natural(this);
    snat->tag |= 1 << SIGNBIT;
    return snat;
}


inline Natural *Natural::MakeUnsigned()
// ----------------------------------------------------------------------------
//   If the number is not unsigned, make a new unsigned version of it
// ----------------------------------------------------------------------------
//   We need to create a copy so that the original number is immutable, as
//   it may have been cached e.g. during evaluation.
{
    if (!IsSigned())
        return this;
    Natural *snat = new Natural(this);
    snat->tag &= ~(1 << SIGNBIT);
    return snat;
}



// ============================================================================
//
//    Utilities
//
// ============================================================================

// Case normalization according to XL language rules
text Normalize(text spelling);
void SourceTree(Tree *tree);



// ============================================================================
//
//    Template members for info management
//
// ============================================================================

template <class I> inline typename I::data_t Tree::Get() const
// ----------------------------------------------------------------------------
//   Find if we have an information of the right type in 'info'
// ----------------------------------------------------------------------------
{
    for (Info *i = info; i; i = i->next)
        if (I *ic = dynamic_cast<I *> (i))
            return (typename I::data_t) *ic;
    return typename I::data_t();
}


template <class I> inline void Tree::Set(typename I::data_t data)
// ----------------------------------------------------------------------------
//   Set the information given as an argument
// ----------------------------------------------------------------------------
{
    Info *i = new I(data);
    // The info can only be owned by a single tree, should not be linked
    XL_ASSERT(Atomic<Tree *>::SetQ(i->owner, nullptr, this));
    LinkedListInsert(info, i);
}


template <class I> inline I* Tree::GetInfo() const
// ----------------------------------------------------------------------------
//   Find if we have an information of the right type in 'info'
// ----------------------------------------------------------------------------
{
    for (Info *i = info; i; i = i->next)
        if (I *ic = dynamic_cast<I *> (i))
            return ic;
    return nullptr;
}


template <class I> inline void Tree::SetInfo(I *i)
// ----------------------------------------------------------------------------
//   Set the information given as an argument
// ----------------------------------------------------------------------------
{
    // The info can only be owned by a single tree, should not be linked
    XL_ASSERT(Atomic<Tree *>::SetQ(i->owner, nullptr, this));
    XL_ASSERT(!i->Info::next);

    Info *asInfo = i;           // For proper deduction if I is a derived class
    LinkedListInsert(info, asInfo);
}


template <class I> inline bool Tree::Exists() const
// ----------------------------------------------------------------------------
//   Verifies if the tree already has information of the given type
// ----------------------------------------------------------------------------
{
    for (Info *i = info; i; i = i->next)
        if (dynamic_cast<I *> (i))
            return true;
    return false;
}


template <class I> inline bool Tree::Purge()
// ----------------------------------------------------------------------------
//   Find and purge information of the given type
// ----------------------------------------------------------------------------
{
retry:
    Info *prev = nullptr;
    Info *next = nullptr;
    bool purged = false;
    for (Info *i = info; i; i = next)
    {
        next = i->next;
        if (I *ic = dynamic_cast<I *> (i))
        {
            if (!Atomic<Info *>::SetQ(i->next, next, nullptr))
                goto retry;
            if (!Atomic<Info *>::SetQ(prev ? prev->next : info, i, next))
                goto retry;
            XL_ASSERT(Atomic<Tree *>::SetQ(i->owner, this, nullptr));
            ic->Delete();
            purged = true;
        }
        else
        {
            prev = i;
        }
    }
    return purged;
}


template <class I> inline I* Tree::Remove()
// ----------------------------------------------------------------------------
//   Find information and unlinks it if it exists
// ----------------------------------------------------------------------------
{
retry:
    Info *prev = nullptr;
    for (Info *i = info; i; i = i->next)
    {
        if (I *ic = dynamic_cast<I *> (i))
        {
            Info *next = i->next;
            if (!Atomic<Info *>::SetQ(i->next, next, nullptr))
                goto retry;
            if (!Atomic<Info *>::SetQ(prev ? prev->next : info, i, next))
                goto retry;
            XL_ASSERT(Atomic<Tree *>::SetQ(i->owner, this, nullptr));
            return ic;
        }
        prev = i;
    }
    return nullptr;
}


template <class I> inline I* Tree::Remove(I *toFind)
// ----------------------------------------------------------------------------
//   Find information matching input and remove it if it exists
// ----------------------------------------------------------------------------
{
retry:
    Info *prev = nullptr;
    for (Info *i = info; i; i = i->next)
    {
        I *ic = dynamic_cast<I *> (i);
        if (ic == toFind)
        {
            Info *next = i->next;
            if (!Atomic<Info *>::SetQ(i->next, next, nullptr))
                goto retry;
            if (!Atomic<Info *>::SetQ(prev ? prev->next : info, i, next))
                goto retry;
            XL_ASSERT(Atomic<Tree *>::SetQ(i->owner, this, nullptr));
            return ic;
        }
        prev = i;
    }
    return nullptr;
}

XL_END

#endif // TREE_H
