//#include <iostream>

#include "anydsl/air/binop.h"
#include "anydsl/air/constant.h"
#include "anydsl/air/type.h"
#include "anydsl/support/universe.h"
#include "anydsl/util/box.h"
#include "anydsl/util/cast.h"
#include "anydsl/util/location.h"
#include "anydsl/util/ops.h"
#include "anydsl/util/types.h"

using namespace anydsl;

int main() {
    std::cout << Location(Position("aaa", 23, 42), Position("bbb", 101, 666)) << std::endl;
    std::cout << Location(Position("aaa", 23, 42), Position("aaa", 101, 666)) << std::endl;
    std::cout << Location(Position("aaa", 23, 42), Position("aaa", 23, 666)) << std::endl;
    std::cout << Location(Position("aaa", 23, 42), Position("aaa", 23, 42)) << std::endl;

    std::cout << std::endl;

    std::cout << Location("aaa:23 col 42 - bbb:101 col 666") << std::endl;
    std::cout << Location("aaa:23 col 42 - 101 col 666") << std::endl;
    std::cout << Location("aaa:23 col 42 - 666") << std::endl;
    std::cout << Location("aaa:23 col 42") << std::endl;

    std::cout << std::endl;

    std::cout << "+" << std::endl;
    std::cout << u1(false) + u1(false) << std::endl;
    std::cout << u1(true) + u1(false) << std::endl;
    std::cout << u1(false) + u1(true) << std::endl;
    std::cout << u1(true) + u1(true) << std::endl;

    std::cout << "*" << std::endl;
    std::cout << u1(false) * u1(false) << std::endl;
    std::cout << u1(true) * u1(false) << std::endl;
    std::cout << u1(false) * u1(true) << std::endl;
    std::cout << u1(true) * u1(true) << std::endl;

    std::cout << "^" << std::endl;
    std::cout << (u1(false) ^ u1(false)) << std::endl;
    std::cout << (u1(true) ^ u1(false)) << std::endl;
    std::cout << (u1(false) ^ u1(true)) << std::endl;
    std::cout << (u1(true) ^ u1(true)) << std::endl;

    {
        std::cout << "+=" << std::endl;
        u1 u(false);
        std::cout << u << std::endl;
        u += 1;
        std::cout << u << std::endl;
        u += 1;
        std::cout << u << std::endl;
    }

    {
        std::cout << "pre ++" << std::endl;
        u1 u(false);
        std::cout << u << std::endl;
        ++u;
        std::cout << u << std::endl;
        ++u;
        std::cout << u << std::endl;
    }

    //ArithOp* p = new ArithOp(ArithOp_add, 0, 0, "todo");
    //switch (p->arithOpKind()) {
        //case ArithOp_ashr
    //}

    //std::cout << p-> << std::endl;
    //

    std::cout << std::endl;
    std::cout << Num_Nodes << std::endl;
    std::cout << Num_ArithOps << std::endl;
    std::cout << Num_RelOps << std::endl;
    std::cout << Num_ConvOps << std::endl;
    std::cout << Num_Indexes << std::endl;

    Universe uni;
    std::cout << uni.get_u8()->debug() << std::endl;

    std::cout << std::endl;

    std::cout << uni.getPrimType(anydsl::PrimType_u1)->debug() << std::endl;
    std::cout << uni.getPrimType(anydsl::PrimType_u64)->debug() << std::endl;
    std::cout << uni.getPrimType(anydsl::PrimType_f32)->debug() << std::endl;
    std::cout << uni.getPrimType(anydsl::PrimType_f64)->debug() << std::endl;

    std::cout << uni.getPrimConst(u32(5)) << std::endl;
#if 0
    std::cout << uni.getPrimType(anydsl::PrimConst_u64)->debug() << std::endl;
    std::cout << uni.getPrimType(anydsl::PrimConst_f32)->debug() << std::endl;
    std::cout << uni.getPrimType(anydsl::PrimConst_f64)->debug() << std::endl;
#endif

    std::cout << std::endl;

    Universe u;
    PrimConst* consts[] = { u.getPrimConst(1u), u.getPrimConst(2ul), u.getPrimConst(3.f) };
    //Tuple* t = new Tuple(u, consts, consts + 3, "fdjk");

    return 0;
}

bool foo(unsigned a, unsigned b) { 
    return anydsl::cmp_sgt(a, b).get();
}
