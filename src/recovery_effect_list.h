#pragma once

// Bounded, read-only capture of the game's native effect list for reload recovery.
// The reader copies memory; this code never dereferences an engine pointer.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <vector>

namespace LootGlowRecovery
{
    using Address = std::uint64_t;
    inline constexpr std::size_t kMaxNodes = 8192;
    inline constexpr std::size_t kMaxCandidates = 512;
    inline constexpr std::uint64_t kBudgetMs = 50;

    enum class Status {
        Checked, UnreadableNode, UnreadableEffect, UnreadableForm,
        InvalidPointer, RepeatedNode, Changed, NodeLimit, CandidateLimit, TimeLimit
    };

    inline const char* StatusName(Status value)
    {
        switch (value) {
        case Status::Checked: return "checked-non-atomic";
        case Status::UnreadableNode: return "unreadable-node";
        case Status::UnreadableEffect: return "unreadable-effect";
        case Status::UnreadableForm: return "unreadable-form";
        case Status::InvalidPointer: return "invalid-pointer";
        case Status::RepeatedNode: return "node-cycle-or-repeat";
        case Status::Changed: return "changed-during-read";
        case Status::NodeLimit: return "node-limit";
        case Status::CandidateLimit: return "candidate-limit";
        default: return "time-limit";
        }
    }

    inline bool AddressRange(Address address, std::size_t size)
    {
        constexpr Address limit = 0x0000800000000000ULL;
        return address >= 0x10000 && address < limit && size <= limit - address && (address % 8) == 0;
    }

    inline bool ExpectedDirectCall(const std::array<std::uint8_t, 5>& bytes,
        Address site, Address target)
    {
        std::int32_t displacement{};
        std::memcpy(&displacement, bytes.data() + 1, sizeof(displacement));
        return bytes[0] == 0xE8 &&
            static_cast<std::int64_t>(site + 5) + displacement == static_cast<std::int64_t>(target);
    }

    enum class InstallGate { Ready, LayoutMismatch, ByteMismatch, RelayFailure, ProtectionFailure, Changed };
    inline InstallGate EvaluateInstall(bool layout, bool bytes, bool relay, bool protection, bool unchanged)
    {
        if (!layout) return InstallGate::LayoutMismatch;
        if (!bytes) return InstallGate::ByteMismatch;
        if (!relay) return InstallGate::RelayFailure;
        if (!protection) return InstallGate::ProtectionFailure;
        return unchanged ? InstallGate::Ready : InstallGate::Changed;
    }

    // Observer failures are isolated from the engine call. The original is not caught,
    // retried, or translated and its complete return value is returned verbatim.
    template<class Original, class Before, class After, class... Args>
    auto ForwardObserved(Original original, Before before, After after, Args... args)
        -> decltype(original(args...))
    {
        try { before(); } catch (...) {}
        auto result = original(args...);
        try { after(result); } catch (...) {}
        return result;
    }

    template<class T, std::size_t N>
    T Field(const std::array<std::uint8_t, N>& bytes, std::size_t offset)
    {
        T value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }

    struct Candidate
    {
        std::size_t ordinal{};
        Address node{}, tempEffect{}, completeEffect{}, target{}, shader{};
        std::uint32_t targetForm{}, shaderForm{};
        float selection{}, duration{}, elapsed{}, shaderElapsed{};
        std::uint8_t finished{}, managed{}, removalLatch{};
        bool targetMatch{}, eligible{}, primary{}, secondary{};
    };

    struct Result
    {
        Status status{Status::Checked};
        std::size_t nodes{}, exactVtables{}, targetMatches{}, eligible{};
        std::size_t primary{}, secondary{}, unknownVtables{};
        Address predicted{};
        std::vector<Candidate> candidates;
    };

    struct NodeCopy
    {
        Address address{}, value{}, next{};
        bool exact{};
        std::array<std::uint8_t, 0x81> effect{};
    };

    template<class Reader, class Clock>
    Result Capture(Address firstNode, Address expectedVtable, Address requestedTarget,
        std::uint32_t primaryShaderForm, std::uint32_t secondaryShaderForm,
        Reader read, Clock now)
    {
        Result out;
        std::vector<NodeCopy> copies;
        copies.reserve(256);
        std::unordered_set<Address> seenNodes;
        const auto started = now();
        float predictedSelection{};
        bool hasPrediction = false;
        Address cursor = firstNode;
        while (cursor) {
            if (now() - started >= kBudgetMs) { out.status = Status::TimeLimit; return out; }
            if (out.nodes >= kMaxNodes) { out.status = Status::NodeLimit; return out; }
            if (!AddressRange(cursor, 16)) { out.status = Status::InvalidPointer; return out; }
            if (!seenNodes.insert(cursor).second) { out.status = Status::RepeatedNode; return out; }

            NodeCopy copy{};
            copy.address = cursor;
            std::array<Address, 2> links{};
            if (!read(cursor, links.data(), sizeof(links))) { out.status = Status::UnreadableNode; return out; }
            copy.value = links[0];
            copy.next = links[1];
            ++out.nodes;

            if (copy.value) {
                Address copiedVtable{};
                if (!AddressRange(copy.value, sizeof(copiedVtable)) || !read(copy.value, &copiedVtable, sizeof(copiedVtable))) {
                    out.status = Status::UnreadableEffect; return out;
                }
                copy.exact = copiedVtable == expectedVtable;
                if (!copy.exact) {
                    ++out.unknownVtables;
                } else {
                    if (out.exactVtables >= kMaxCandidates) { out.status = Status::CandidateLimit; return out; }
                    if (!AddressRange(copy.value, copy.effect.size()) ||
                        !read(copy.value, copy.effect.data(), copy.effect.size()) ||
                        Field<Address>(copy.effect, 0) != expectedVtable) {
                        out.status = Status::UnreadableEffect; return out;
                    }
                    Candidate candidate{};
                    candidate.ordinal = out.nodes - 1;
                    candidate.node = cursor;
                    candidate.tempEffect = copy.value;
                    candidate.completeEffect = copy.value - 0x18;
                    candidate.duration = Field<float>(copy.effect, 0x10);
                    candidate.selection = Field<float>(copy.effect, 0x20);
                    candidate.target = Field<Address>(copy.effect, 0x30);
                    candidate.elapsed = Field<float>(copy.effect, 0x38);
                    candidate.finished = copy.effect[0x3c];
                    candidate.managed = copy.effect[0x40];
                    candidate.shader = Field<Address>(copy.effect, 0x50);
                    candidate.shaderElapsed = Field<float>(copy.effect, 0x58);
                    candidate.removalLatch = copy.effect[0x80];
                    if (candidate.target &&
                        (!AddressRange(candidate.target, 0x14) || !read(candidate.target + 0x10, &candidate.targetForm, 4))) {
                        out.status = Status::UnreadableForm; return out;
                    }
                    if (candidate.shader &&
                        (!AddressRange(candidate.shader, 0x14) || !read(candidate.shader + 0x10, &candidate.shaderForm, 4))) {
                        out.status = Status::UnreadableForm; return out;
                    }
                    candidate.targetMatch = candidate.target == requestedTarget;
                    candidate.eligible = candidate.targetMatch && candidate.managed == 0;
                    candidate.primary = candidate.shaderForm == primaryShaderForm;
                    candidate.secondary = candidate.shaderForm == secondaryShaderForm;
                    ++out.exactVtables;
                    if (candidate.targetMatch) ++out.targetMatches;
                    if (candidate.eligible) {
                        ++out.eligible;
                        // Strictly lower only: equal values preserve the first candidate.
                        if (!hasPrediction || candidate.selection < predictedSelection) {
                            hasPrediction = true;
                            predictedSelection = candidate.selection;
                            out.predicted = candidate.completeEffect;
                        }
                    }
                    if (candidate.primary) ++out.primary;
                    if (candidate.secondary) ++out.secondary;
                    out.candidates.push_back(candidate);
                }
            }
            copies.push_back(copy);
            cursor = copy.next;
        }

        // Recheck copied links and exact candidate bytes to detect observable mutation.
        for (const auto& copy : copies) {
            if (now() - started >= kBudgetMs) { out.status = Status::TimeLimit; return out; }
            std::array<Address, 2> links{};
            if (!read(copy.address, links.data(), sizeof(links))) { out.status = Status::UnreadableNode; return out; }
            if (links[0] != copy.value || links[1] != copy.next) { out.status = Status::Changed; return out; }
            if (copy.exact) {
                std::array<std::uint8_t, 0x81> effect{};
                if (!read(copy.value, effect.data(), effect.size())) { out.status = Status::UnreadableEffect; return out; }
                if (effect != copy.effect) { out.status = Status::Changed; return out; }
            }
        }
        return out;
    }
}
