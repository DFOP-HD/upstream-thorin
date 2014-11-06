#include "thorin/analyses/cfg.h"
#include "thorin/analyses/scope.h"
#include "thorin/analyses/domtree.h"
#include "thorin/analyses/looptree.h"

#include <iostream>

namespace thorin {

//------------------------------------------------------------------------------

CFG::CFG(const Scope& scope) 
    : scope_(scope)
    //, nodes_(scope.size() + 1)                      // one extra alloc for virtual exit
    , nodes_(scope.size())
{
    for (size_t i = 0, e = size(); i != e; ++i)
        nodes_[i] = new CFGNode(scope[i]);
    //nodes_.back() = new CFGNode(nullptr);           // virtual exit

    //for (auto n : nodes_.slice_num_from_end(1)) {  // skip virtual exit
    for (auto n : nodes_) {
        for (auto succ : n->lambda()->succs()) {
            if (scope.contains(succ))
                link(n, nodes_[sid(succ)]);
        }
    }
}

size_t CFG::sid(Lambda* lambda) const { 
    return lambda == nullptr ? nodes_.size()-1 : lambda->find_scope(&scope())->sid; 
}

struct FlowVal {
    LambdaSet lambdas;
    bool top = false;
    bool join(const FlowVal& other) {
        bool result = false;
        for (auto l : other.lambdas)
            result |= this->lambdas.insert(l).second;
        return result;
    }
};

void CFG::cfa() {
    DefMap<FlowVal> param2fv;

    // init
    for (auto lambda : scope()) {
        if (!lambda->empty()) {
            if (auto to = lambda->to()->isa_lambda()) {
                for (size_t i = 0, e = lambda->num_args(); i != e; ++i) {
                    if (auto arg = lambda->arg(i)->isa_lambda())
                        param2fv[to->param(i)].lambdas.insert(arg);
                }
            }
        }
    }

    // keep iterating to collect param flow infos until things are stable
    bool todo;
    do {
        todo = false;
        for (auto lambda : scope()) {
            if (auto to = lambda->to()->isa_lambda()) {
                for (size_t i = 0, e = lambda->num_args(); i != e; ++i) {
                    if (auto arg = lambda->arg(i)->isa<Param>())
                        todo |= param2fv[to->param(i)].join(param2fv[arg]);
                }
            }
        }
    } while (todo);
}

const F_CFG* CFG::f_cfg() const { return lazy_init(this, f_cfg_); }
const B_CFG* CFG::b_cfg() const { return lazy_init(this, b_cfg_); }
const DomTree* CFG::domtree() const { return f_cfg()->domtree(); }
const PostDomTree* CFG::postdomtree() const { return b_cfg()->domtree(); }
const LoopTree* CFG::looptree() const { return looptree_ ? looptree_ : looptree_ = new LoopTree(*f_cfg()); }

//------------------------------------------------------------------------------

template<bool forward>
CFGView<forward>::CFGView(const CFG& cfg)
    : cfg_(cfg)
    , rpo_ids_(cfg.size())
    , rpo_(cfg.nodes()) // copy over - sort later
{
    std::fill(rpo_ids_.begin(), rpo_ids_.end(), -1); // mark as not visited
    auto num = number(entry(), size());
    assert(num == 0);

    // sort in reverse post-order
    std::sort(rpo_.begin(), rpo_.end(), [&] (const CFGNode* n1, const CFGNode* n2) { return rpo_id(n1) < rpo_id(n2); });
}

template<bool forward>
size_t CFGView<forward>::number(const CFGNode* n, size_t i) {
    auto& n_rpo_id = _rpo_id(n);
    n_rpo_id = -2; // mark as visited

    for (auto succ : succs(n)) {
        if (rpo_id(succ) == size_t(-1)) // if not visited
            i = number(succ, i);
    }

    return n_rpo_id = i-1;
}

template<bool forward>
const DomTreeBase<forward>* CFGView<forward>::domtree() const { return lazy_init(this, domtree_); }

//------------------------------------------------------------------------------

}
