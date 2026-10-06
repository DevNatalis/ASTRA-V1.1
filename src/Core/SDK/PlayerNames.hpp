#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace PlayerNames {
    // FiveM usa server-ids de 32 bits que crescem com o tempo (#158760 visto
    // em produção). Qualquer teto de 16 bits (65535) zera o ESP Name no
    // servidor cheio. 16M cobre qualquer id real e ainda rejeita lixo.
    static constexpr int kMaxServerId = 16777215;
    // Onesync suporta até ~2048 slots; folga para não descartar lista cheia.
    static constexpr uint64_t kMaxPlayers = 2048;

    inline bool LooksLikeName(const char* data, size_t size) {
        if (size < 1 || size > 256) return false;
        for (size_t i = 0; i < size; ++i) {
            unsigned char ch = static_cast<unsigned char>(data[i]);
            // >= 32 e != 127; bytes >= 128 passam (UTF-8: acentos em nomes).
            if (ch < 32 || ch == 127) return false;
        }
        return true;
    }

    // Layout (MSVC x64, citizen-playernames-five.dll):
    //   node+0x00: next | +0x08: prev | +0x10: id (int) |
    //   +0x18: std::string SSO(16)/ptr | +0x28: size | +0x30: capacity
    //
    // Leitura TOLERANTE: aproveita cada node válido e pula os ruins.
    // A versão anterior invalidava a lista INTEIRA no primeiro node estranho
    // (um id > 65535 bastava) e com 468 online o mapa ficava sempre vazio.
    template<class Reader>
    bool ReadList(Reader&& read, uintptr_t entry, std::unordered_map<int, std::string>& output) {
        uint64_t header[2]{};
        if (!read(entry, header, sizeof(header))) return false;
        const uintptr_t head = header[0];
        const uint64_t count = header[1];
        if (head < 0x10000 || head > 0x7FFFFFFFFFFFULL) return false;
        if (count == 0 || count > kMaxPlayers) return false;
        uintptr_t node = 0;
        if (!read(head, &node, sizeof(node))) return false;

        std::unordered_map<int, std::string> fresh;
        std::unordered_set<uintptr_t> visited;
        for (uint64_t i = 0; i < count; ++i) {
            // Ponteiro ruim ou corrente que fechou: para e mantém o coletado.
            if (node < 0x10000 || node > 0x7FFFFFFFFFFFULL || node == head ||
                !visited.insert(node).second)
                break;
            unsigned char raw[56]{};
            if (!read(node, raw, sizeof(raw))) break;

            uintptr_t next = 0, storage = 0;
            uint64_t size = 0, capacity = 0;
            int id = 0;
            std::memcpy(&next, raw, 8);
            std::memcpy(&id, raw + 16, 4);
            std::memcpy(&storage, raw + 24, 8);
            std::memcpy(&size, raw + 40, 8);
            std::memcpy(&capacity, raw + 48, 8);

            // Node fora do esperado? Pula ele, não a lista toda.
            if (id > 0 && id <= kMaxServerId && size >= 1 && size <= 256 &&
                capacity >= size && capacity <= 8192 &&
                (capacity >= 16 || capacity == 15)) {
                if (capacity < 16) storage = node + 24; // SSO inline
                if (storage >= 0x10000 && storage <= 0x7FFFFFFFFFFFULL) {
                    std::string name(static_cast<size_t>(size) + 1, '\0');
                    if (read(storage, name.data(), name.size()) && name.back() == '\0') {
                        name.pop_back();
                        if (LooksLikeName(name.data(), name.size()))
                            fresh.emplace(id, std::move(name));
                    }
                }
            }
            if (next == 0 || next == node) break;
            node = next;
        }
        if (fresh.empty()) return false;
        output.swap(fresh);
        return true;
    }
}
