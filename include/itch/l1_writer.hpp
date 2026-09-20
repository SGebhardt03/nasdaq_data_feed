//
// Created by samuel-gebhardt on 21.09.26.
//
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "messages.hpp"
#include "dispatch.hpp"
#include "book.hpp"
#include "l1_writer.hpp"

class L1Writer {
    public:
        L1Writer();

        void emit(uint64_t ts_ns, uint16_t locate, const data_feed::TopOfBook& tob) {

        }

    private:

};
