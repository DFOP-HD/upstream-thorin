#include "anydsl/world.h"

#include <queue>
#include <boost/unordered_set.hpp>

#include "anydsl/def.h"
#include "anydsl/primop.h"
#include "anydsl/lambda.h"
#include "anydsl/literal.h"
#include "anydsl/type.h"
#include "anydsl/fold.h"

namespace anydsl {

/*
 * helpers
 */

static inline bool isCommutative(ArithOpKind kind) {
    switch (kind) {
        case ArithOp_add:
        case ArithOp_mul:
            return true;
        default:
            return false;
    }
}

static inline RelOpKind normalizeRel(RelOpKind kind, bool& swap) {
    swap = false;
    switch (kind) {
        case RelOp_cmp_ugt: swap = true; return RelOp_cmp_ult;
        case RelOp_cmp_uge: swap = true; return RelOp_cmp_ule;
        case RelOp_cmp_sgt: swap = true; return RelOp_cmp_slt;
        case RelOp_cmp_sge: swap = true; return RelOp_cmp_sle;

        case RelOp_fcmp_ogt: swap = true; return RelOp_fcmp_olt;
        case RelOp_fcmp_oge: swap = true; return RelOp_fcmp_ole;
        case RelOp_fcmp_ugt: swap = true; return RelOp_fcmp_ult;
        case RelOp_fcmp_uge: swap = true; return RelOp_fcmp_ule;
        default: return kind;
    }
}

static void examineDef(const Def* def, FoldValue& v) {
    if (def->isa<Undef>())
        v.kind = FoldValue::Undef;
    else if (def->isa<ErrorLit>())
        v.kind = FoldValue::Error;
    if (const PrimLit* lit = def->isa<PrimLit>()) {
        v.kind = FoldValue::Valid;
        v.box = lit->box();
    }
   
}

/*
 * constructor and destructor
 */

World::World() 
    : defs_(1031)
    , unit_ (find(new Sigma(*this, (const Type* const*) 0, (const Type* const*) 0)))
    , pi0_  (find(new Pi   (*this, (const Type* const*) 0, (const Type* const*) 0)))
#define ANYDSL_U_TYPE(T) ,T##_(find(new PrimType(*this, PrimType_##T)))
#define ANYDSL_F_TYPE(T) ,T##_(find(new PrimType(*this, PrimType_##T)))
#include "anydsl/tables/primtypetable.h"
{
    live_.insert(unit_);
    live_.insert(pi0_);
    for (size_t i = 0; i < Num_PrimTypes; ++i)
        live_.insert(primTypes_[i]);
}

World::~World() {
    live_.clear();
    reachable_.clear();
    cleanup();

    anydsl_assert(defs_.empty(), "cleanup should catch everything");
}

/*
 * types
 */

Sigma* World::namedSigma(size_t num, const std::string& name /*= ""*/) {
    Sigma* s = new Sigma(*this, num);
    s->debug = name;

    anydsl_assert(defs_.find(s) == defs_.end(), "must not be inside");
    defs_.insert(s);

    return s;
}

/*
 * literals
 */

const PrimLit* World::literal(PrimLitKind kind, Box value) {
    return find(new PrimLit(type(lit2type(kind)), value));
}

const PrimLit* World::literal(const PrimType* p, Box value) {
    return find(new PrimLit(p, value));
}

const Undef* World::undef(const Type* type) {
    return find(new Undef(type));
}

const ErrorLit* World::literal_error(const Type* type) {
    return find(new ErrorLit(type));
}

/*
 * create
 */

const Def* World::createTuple(const Def* const* begin, const Def* const* end) { 
    return find(new Tuple(*this, begin, end));
}

const Def* World::tryFold(IndexKind kind, const Def* ldef, const Def* rdef) {
    FoldValue a(ldef->type()->as<PrimType>()->kind());
    FoldValue b(a.type);

    examineDef(ldef, a);
    examineDef(rdef, b);

    if (ldef->isa<Literal>() && rdef->isa<Literal>()) {
        const PrimType* p = ldef->type()->as<PrimType>();
        FoldValue res = fold_bin(kind, p->kind(), a, b);

        switch (res.kind) {
            case FoldValue::Valid: return literal(res.type, res.box);
            case FoldValue::Undef: return undef(res.type);
            case FoldValue::Error: return literal_error(res.type);
        }
    }

    return 0;
}

const Def* World::createArithOp(ArithOpKind kind, const Def* ldef, const Def* rdef) {
    if (const Def* value = tryFold((IndexKind) kind, ldef, rdef))
        return value;

    if (isCommutative(kind))
        if (ldef > rdef)
            std::swap(ldef, rdef);

    return find(new ArithOp(kind, ldef, rdef));
}

const Def* World::createRelOp(RelOpKind kind, const Def* ldef, const Def* rdef) {
    if (const Def* value = tryFold((IndexKind) kind, ldef, rdef))
        return value;

    bool swap;
    kind = normalizeRel(kind, swap);
    if (swap)
        std::swap(ldef, rdef);

    return find(new RelOp(kind, ldef, rdef));
}

const Def* World::createExtract(const Def* tuple, size_t index) {
    // TODO folding
    return find(new Extract(tuple, index));
}

const Def* World::createInsert(const Def* tuple, size_t index, const Def* value) {
    // TODO folding
    return find(new Insert(tuple, index, value));
}


const Def* World::createSelect(const Def* cond, const Def* tdef, const Def* fdef) {
    if (const PrimLit* lit = cond->isa<PrimLit>())
        return lit->box().get_u1().get() ? tdef : fdef;

    return find(new Select(cond, tdef, fdef));
}

const Lambda* World::finalize(const Lambda* lambda) {
    anydsl_assert(lambda->type(), "must be set");
    anydsl_assert(lambda->pi(),   "must be a set pi type");

    const Lambda* l = find<Lambda>(lambda);
    assert(l == lambda);
    assert(defs_.find(l) != defs_.end());

    return l;
}

const Param* World::createParam(const Type* type, const Lambda* parent, size_t index) {
    return find(new Param(type, parent, index));
}

/*
 * optimizations
 */

void World::setLive(const Def* def) {
    live_.insert(def);
}

void World::setReachable(const Lambda* lambda) {
    assert(defs_.find(lambda) != defs_.end());
    reachable_.insert(lambda);
}

void World::dce() {
    // mark all as dead
    unmark();

    // find all live values
    for_all (def, live_)
        dce_insert(def);

    // kill the living dead
    DefSet::iterator i = defs_.begin();
    while (i != defs_.end()) {
        const Def* def = *i;
        if (!def->flag_) {
            delete def;
            i = defs_.erase(i);
        } else
            ++i;
    }
}

void World::dce_insert(const Def* def) {
    if (def->flag_)
        return;

    def->flag_ = true;

    if (const Type* type = def->type())
        dce_insert(type);

    if (const Param* param = def->isa<Param>()) {
        param->lambda()->flag_ = true;

        for_all (op, param->phiOps()) {
            // look through "phi-args"
            dce_insert(op.def());

            // insert control-dependent lambdas
            for_all (caller, op.from()->callers())
                dce_insert(caller);
        }

        return;
    }

    for_all (op, def->ops())
        dce_insert(op);
}

void World::uce() {
    Reachable reachable;

    // mark all as unreachable
    unmark();

    // find all reachable lambdas
    for_all (lambda, reachable_)
        uce_insert(reachable, lambda);

    // destroy all unreachable lambdas
    DefSet::iterator i = defs_.begin();
    while (i != defs_.end()) {
        if (const Lambda* lambda = (*i)->isa<Lambda>()) {
            if (!lambda->flag_) {
                delete lambda;
                i = defs_.erase(i);
                continue;
            }
        }
        ++i;
    }
}

void World::uce_insert(Reachable& reachable, const Lambda* lambda) {
    assert(defs_.find(lambda) != defs_.end());

    if (lambda->flag_)
        return;

    lambda->flag_ = true;
    reachable.insert(lambda);

    for_all (succ, lambda->succ())
        uce_insert(reachable, succ);
}

void World::cleanup() {
    uce();
    dce();
}

void World::unmark() {
    for_all (def, defs_)
        def->flag_ = false;
}

const Def* World::findDef(const Def* def) {
    DefSet::iterator i = defs_.find(def);
    if (i != defs_.end()) {
        anydsl_assert(!def->isa<Lambda>(), "must not be a lambda");
        delete def;
        anydsl_assert(defs_.find(*i) != defs_.end(), "hash/equal function of def class incorrect");
        return *i;
    }

    defs_.insert(def);
    anydsl_assert(defs_.find(def) != defs_.end(), "hash/equal function of def class incorrect");

    return def;
}

void World::dump(bool fancy) {
    unmark();

    for_all (def, defs_) {
        if (const Lambda* lambda = def->isa<Lambda>()) {
            if (lambda->flag_)
                continue;

            std::queue<const Lambda*> queue;
            queue.push(lambda);
            lambda->flag_ = true;

            while (!queue.empty()) {
                const Lambda* cur = queue.front();
                queue.pop();

                cur->dump(fancy);
                std::cout << std::endl;

                for_all (succ, cur->succ()) {
                    if (!succ->flag_) {
                        succ->flag_ = true;
                        queue.push(succ);
                    }
                }
            }
        }
    }
}

Params World::findParams(const Lambda* lambda) {
    Params result;

    const Pi* pi = lambda->pi();
    size_t num = pi->numOps();

    for (size_t i = 0; i < num; ++i) {
        Param param(pi->get(i), lambda, i);

        DefSet::iterator j = defs_.find(&param);
        if (j != defs_.end())
            result.push_back((*j)->as<Param>());
    }

    return result;
}

} // namespace anydsl
