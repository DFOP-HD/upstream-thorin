#ifndef ANYDSL_JUMP_H
#define ANYDSL_JUMP_H

#include "anydsl/airnode.h"
#include "anydsl/lambda.h"
#include "anydsl/def.h"

namespace anydsl {

class NoRet;

class Jump : public Def {
protected:

    Jump(World& world, IndexKind indexKind, size_t numOps);
};

//------------------------------------------------------------------------------

class Goto : public Jump {
private:

    Goto(World& world, const Def* to, const Def* const* begin, const Def* const* end);

public:

    const Lambda* toLambda() const { return to()->isa<Lambda>(); }

    const NoRet* noret() const;

    struct Args {
        typedef const Def** const_iterator;
        typedef std::reverse_iterator<const Def**> const_reverse_iterator;

        Args(const Goto& jump) : jump(jump) {}

        const_iterator begin() const { return jump.ops_ + 1; }
        const_iterator end() const { return jump.ops_ + 1 + size(); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(jump.ops_ + 1 + size()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(jump.ops_ + 1); }

        const Def* const& operator [] (size_t i) const {
            anydsl_assert(i < size(), "index out of bounds");
            return jump.ops_[i + 1];
        }

        size_t size() const { return jump.numOps() - 1; }
        bool empty() const { return jump.numOps() == 1; }

        const Def*& front() const { return *(jump.ops_ + 1); }
        const Def*& back() const { return *(jump.ops_ + 1 + size() - 1); }

        const Goto& jump;
    };

    Args args() const { return Args(*this); }

    virtual void dump(Printer& printer, LambdaPrinterMode mode) const;

    const Def* to() const { return ops_[0]; };

private:

    friend class World;
    friend class Args;
};

} // namespace anydsl

#endif
