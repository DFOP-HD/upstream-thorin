#ifndef THORIN_BE_LLVM_AMDGPU_H
#define THORIN_BE_LLVM_AMDGPU_H

#include "thorin/be/llvm/llvm.h"

namespace thorin {

class Load;

namespace llvm {

namespace llvm = ::llvm;

class AMDGPUCodeGen : public CodeGen {
public:
    AMDGPUCodeGen(World& world, const Cont2Config&, int opt, bool debug);

    const char* file_ext() const override { return ".amdgpu"; }

protected:
    virtual void emit_function_decl_hook(Continuation*, llvm::Function*) override;
    virtual llvm::Function* emit_function_decl(Continuation*) override;
    virtual llvm::Value* emit_global(const Global*) override;
    virtual Continuation* emit_reserve(llvm::IRBuilder<>&, const Continuation*) override;
    virtual std::string get_alloc_name() const override { return "malloc"; }

    const Cont2Config& kernel_config_;
};

}

}

#endif
