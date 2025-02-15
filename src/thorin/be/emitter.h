#ifndef THORIN_BE_EMITTER_H
#define THORIN_BE_EMITTER_H

namespace thorin {

template<class Value, class Type, class BB, class Child>
class Emitter {
private:
    constexpr const Child& child() const { return *static_cast<const Child*>(this); };
    constexpr Child& child() { return *static_cast<Child*>(this); };

    /// Internal wrapper for @p emit that checks and retrieves/puts the @c Value from @p defs_.
    Value emit_(const Def* def) {
        auto place = def->no_dep() ? entry_ : scheduler_.smart(def);
        auto& bb = cont2bb_[place];
        return child().emit_bb(bb, def);
    }

protected:
    //@{
    /// @name default implementations
    void finalize(const Scope&) {}
    void finalize(Continuation*) {}
    //@}

    /// Recursively emits code. @c mem -typed @p Def%s return sth that is @c !child().is_valid(value) - this variant asserts in this case.
    Value emit(const Def* def) {
        auto res = emit_unsafe(def);
        assert(child().is_valid(res));
        return res;
    }

    /// As above but returning @c !child().is_valid(value) is permitted.
    Value emit_unsafe(const Def* def) {
        if (auto val = defs_.lookup(def)) return *val;
        if (auto cont = def->isa_nom<Continuation>()) return defs_[cont] = child().emit_fun_decl(cont);

        auto val = emit_(def);
        return defs_[def] = val;
    }

    void emit_scope(const Scope& scope) {
        auto conts = schedule(scope);
        entry_ = scope.entry();
        assert(entry_->is_returning());

        auto fct = child().prepare(scope);
        for (auto cont : conts) {
            if (cont->intrinsic() != Intrinsic::EndScope) child().prepare(cont, fct);
        }

        Scheduler new_scheduler(scope);
        swap(scheduler_, new_scheduler);

        for (auto cont : conts) {
            if (cont->intrinsic() == Intrinsic::EndScope) continue;
            assert(cont == entry_ || cont->is_basicblock());
            child().emit_epilogue(cont);
        }

        for (auto cont : conts) {
            if (cont->intrinsic() != Intrinsic::EndScope) child().finalize(cont);
        }
        child().finalize(scope);
    }

    Scheduler scheduler_;
    DefMap<Value> defs_;
    TypeMap<Type> types_;
    ContinuationMap<BB> cont2bb_;
    Continuation* entry_ = nullptr;
};

}

#endif
