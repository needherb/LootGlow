#include "recovery_effect_list.h"

#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace
{
    using LootGlowRecovery::Address;
    constexpr Address vtable = 0x1486611A8;
    constexpr Address target = 0x20000;
    constexpr Address otherTarget = 0x21000;
    constexpr std::uint32_t primary = 0x000C793E;
    constexpr std::uint32_t secondary = 0x000C793F;

    struct Memory
    {
        std::unordered_map<Address, std::uint8_t> bytes;
        Address unreadable{};
        bool mutateNode{};
        mutable unsigned nodeReads{};

        template<class T> void put(Address address, const T& value)
        {
            const auto* source = reinterpret_cast<const std::uint8_t*>(&value);
            for (std::size_t i = 0; i < sizeof(value); ++i) bytes[address + i] = source[i];
        }
        bool read(Address address, void* output, std::size_t size) const
        {
            if (unreadable && address <= unreadable && unreadable < address + size) return false;
            auto* destination = static_cast<std::uint8_t*>(output);
            for (std::size_t i = 0; i < size; ++i) {
                const auto found = bytes.find(address + i);
                if (found == bytes.end()) return false;
                destination[i] = found->second;
            }
            if (mutateNode && address == 0x10000 && size == 16 && ++nodeReads == 2)
                destination[8] ^= 1;
            return true;
        }
    };

    void form(Memory& memory, Address address, std::uint32_t id) { memory.put(address + 0x10, id); }

    void effect(Memory& memory, Address address, Address effectVtable, Address effectTarget,
        Address shader, float rank, std::uint8_t managed = 0)
    {
        std::array<std::uint8_t, 0x81> data{};
        auto field = [&](std::size_t offset, const auto& value) { std::memcpy(data.data() + offset, &value, sizeof(value)); };
        field(0, effectVtable);
        const float duration = 12.0f, elapsed = 3.0f, shaderElapsed = 2.0f;
        field(0x10, duration); field(0x20, rank); field(0x30, effectTarget); field(0x38, elapsed);
        data[0x3c] = 0; data[0x40] = managed; field(0x50, shader); field(0x58, shaderElapsed); data[0x80] = 1;
        for (std::size_t i = 0; i < data.size(); ++i) memory.bytes[address + i] = data[i];
    }

    void node(Memory& memory, Address address, Address value, Address next)
    {
        std::array<Address, 2> links{value, next}; memory.put(address, links);
    }

    LootGlowRecovery::Result capture(Memory& memory, Address head = 0x10000)
    {
        std::uint64_t time = 0;
        return LootGlowRecovery::Capture(head, vtable, target, primary, secondary,
            [&](Address a, void* p, std::size_t n) { return memory.read(a, p, n); },
            [&]() { return time; });
    }

    void commonForms(Memory& memory)
    {
        form(memory, target, 0x0015983C); form(memory, otherTarget, 0x00159836);
        form(memory, 0x30000, primary); form(memory, 0x31000, secondary);
    }
}

int main()
{
    { std::array<std::uint8_t,5> call{0xE8,0,0,0,0}; std::int32_t d=0x20000-(0x10000+5); std::memcpy(call.data()+1,&d,4); assert(LootGlowRecovery::ExpectedDirectCall(call,0x10000,0x20000)); call[0]=0x90; assert(!LootGlowRecovery::ExpectedDirectCall(call,0x10000,0x20000)); }
    { assert(LootGlowRecovery::EvaluateInstall(true,true,true,false,true)==LootGlowRecovery::InstallGate::ProtectionFailure); assert(LootGlowRecovery::EvaluateInstall(true,false,true,true,true)==LootGlowRecovery::InstallGate::ByteMismatch); }
    { int calls=0, before=0, after=0; int marker=7; auto* returned=LootGlowRecovery::ForwardObserved([&](int value){++calls; assert(value==4); return &marker;},[&](){++before; throw 1;},[&](int* value){++after; assert(value==&marker); throw 2;},4); assert(returned==&marker && calls==1 && before==1 && after==1); }
    { Memory m; node(m, 0x10000, 0, 0); auto r = capture(m); assert(r.status == LootGlowRecovery::Status::Checked && r.nodes == 1 && !r.predicted); }
    { Memory m; commonForms(m); node(m,0x10000,0x40018,0); effect(m,0x40018,vtable,target,0x30000,5); auto r=capture(m); assert(r.predicted==0x40000 && r.eligible==1); }
    { Memory m; commonForms(m); node(m,0x10000,0x40018,0x11000); node(m,0x11000,0x41018,0); effect(m,0x40018,vtable,target,0x30000,5); effect(m,0x41018,vtable,target,0x31000,5); auto r=capture(m); assert(r.predicted==0x40000); }
    { Memory m; commonForms(m); node(m,0x10000,0x40018,0x11000); node(m,0x11000,0x41018,0); effect(m,0x40018,vtable,target,0x30000,5); effect(m,0x41018,vtable,target,0x31000,4); auto r=capture(m); assert(r.predicted==0x41000); }
    { Memory m; commonForms(m); node(m,0x10000,0x40018,0); effect(m,0x40018,vtable,otherTarget,0x30000,1); auto r=capture(m); assert(!r.predicted && r.targetMatches==0); }
    { Memory m; commonForms(m); node(m,0x10000,0x40018,0); effect(m,0x40018,vtable,target,0x30000,1,1); auto r=capture(m); assert(!r.predicted && r.eligible==0); }
    { Memory m; node(m,0x10000,0x40018,0); Address unknown=0x12345678; m.put(0x40018,unknown); auto r=capture(m); assert(r.status==LootGlowRecovery::Status::Checked && r.unknownVtables==1 && r.candidates.empty()); }
    { Memory m; node(m,0x10000,0,0); m.unreadable=0x10008; auto r=capture(m); assert(r.status==LootGlowRecovery::Status::UnreadableNode); }
    { Memory m; node(m,0x10000,0x40018,0); m.unreadable=0x40018; auto r=capture(m); assert(r.status==LootGlowRecovery::Status::UnreadableEffect); }
    { Memory m; commonForms(m); node(m,0x10000,0x40018,0); effect(m,0x40018,vtable,target,0x30000,1); m.unreadable=0x30010; auto r=capture(m); assert(r.status==LootGlowRecovery::Status::UnreadableForm); }
    { Memory m; node(m,0x10000,0,0x10000); auto r=capture(m); assert(r.status==LootGlowRecovery::Status::RepeatedNode); }
    { Memory m; node(m,0x10000,0,0); m.mutateNode=true; auto r=capture(m); assert(r.status==LootGlowRecovery::Status::Changed); }
    { Memory m; for(std::size_t i=0;i<LootGlowRecovery::kMaxNodes+1;++i) node(m,0x10000+i*0x100,0,0x10000+(i+1)*0x100); auto r=capture(m); assert(r.status==LootGlowRecovery::Status::NodeLimit); }
    { Memory m; commonForms(m); for(std::size_t i=0;i<LootGlowRecovery::kMaxCandidates+1;++i){Address n=0x10000+i*0x100,e=0x100000+i*0x100;node(m,n,e,i==LootGlowRecovery::kMaxCandidates?0:n+0x100);effect(m,e,vtable,target,0x30000,static_cast<float>(i));} auto r=capture(m); assert(r.status==LootGlowRecovery::Status::CandidateLimit); }
    return 0;
}
