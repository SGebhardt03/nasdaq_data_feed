#pragma once

#include <zlib.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "endian.hpp"  // df::read_be<std::uint16_t>

namespace df {

// Reads a NASDAQ binary file (TotalView-ITCH 5.0)
// and returns complete messages as span.
//
// Framing: every message is prefixed with 2-byte big endian length.
class StreamReader {
 public:
  static constexpr std::size_t kDefaultCapacity = 1u << 20;  // 1 MiB
  static constexpr std::size_t kLenPrefix = 2;

  explicit StreamReader(const std::string& path,
                        std::size_t capacity = kDefaultCapacity)
      : buf_(capacity) {

    using_memory_ = false;

    if (capacity < kLenPrefix + 1) {
      throw std::invalid_argument("StreamReader: capacity too small");
    }
    file_ = ::gzopen(path.c_str(), "rb");
    if (file_ == nullptr) {
      throw std::runtime_error("gzopen failed: " + path);
    }
    ::gzbuffer(file_, 1u << 17);  // 128 KiB internal inflate-buffer
  }

  explicit StreamReader(std::vector<std::byte> data, std::size_t capacity = kDefaultCapacity) : buf_(capacity) {
    using_memory_ = true;

    if (capacity < kLenPrefix + 1) {
      throw std::invalid_argument("StreamReader: capacity too small");
    }
    mem_source_ = std::move(data);

    file_ = nullptr; // should already be set
  }

  ~StreamReader() {
    if (file_ != nullptr) ::gzclose(file_);  // ignores return
  }

  StreamReader(const StreamReader&) = delete;
  StreamReader& operator=(const StreamReader&) = delete;

  StreamReader(StreamReader&& o) noexcept
      : file_(std::exchange(o.file_, nullptr)),
        buf_(std::move(o.buf_)),
        head_(std::exchange(o.head_, 0)),
        tail_(std::exchange(o.tail_, 0)),
        consumed_(std::exchange(o.consumed_, 0)),
        eof_(std::exchange(o.eof_, true)) {}

  StreamReader& operator=(StreamReader&& o) noexcept {
    if (this != &o) {
      if (file_ != nullptr) ::gzclose(file_);
      file_ = std::exchange(o.file_, nullptr);
      buf_ = std::move(o.buf_);
      head_ = std::exchange(o.head_, 0);
      tail_ = std::exchange(o.tail_, 0);
      consumed_ = std::exchange(o.consumed_, 0);
      eof_ = std::exchange(o.eof_, true);
    }
    return *this;
  }

  // Next message without prefix. empty span == regular end of stream.
  // Throws, if stream ends within message.
  [[nodiscard]] std::span<const std::byte> next() {
    if (!ensure(kLenPrefix)) {
      if (available() != 0) throw truncated("length prefix");
      return {};
    }
    const auto len = read_be<std::uint16_t>(view(head_, kLenPrefix));

    // BinaryFILE marks end of file with a message of length 0.
    if (len == 0) {
      head_ += kLenPrefix;
      consumed_ += kLenPrefix;
      return {};
    }

    const std::size_t total = kLenPrefix + len;
    if (total > buf_.size()) {
      throw std::runtime_error("Message longer as buffer (" +
                               std::to_string(total) + " Byte)");
    }
    if (!ensure(total)) throw truncated("Message-Body");

    const auto msg = view(head_ + kLenPrefix, len);
    head_ += total;
    consumed_ += total;
    return msg;
  }

  // Byte offset in the *decompressed* stream -- for error messages/progress.
  [[nodiscard]] std::uint64_t offset() const noexcept { return consumed_; }

 private:
  [[nodiscard]] std::size_t available() const noexcept { return tail_ - head_; }

  [[nodiscard]] std::span<const std::byte> view(std::size_t off,
                                                std::size_t n) const noexcept {
    return std::span<const std::byte>{buf_}.subspan(off, n);
  }

  [[nodiscard]] std::runtime_error truncated(const char* what) const {
    return std::runtime_error(std::string("Stream ended during ") + what +
                              " at offset " + std::to_string(consumed_));
  }

  // Guarantees >= n connected byte after head_
  bool ensure(std::size_t n) {
    while (available() < n) {
      if (!fill()) return false;
    }
    return true;
  }

  // shift forward and load
  bool fill() {
    if (eof_) return false;

    if (head_ != 0) {
      const std::size_t rest = available();
      if (rest != 0) std::memmove(buf_.data(), buf_.data() + head_, rest);
      head_ = 0;
      tail_ = rest;
    }

    const std::size_t space = buf_.size() - tail_;
    if (space == 0) return false;  // from next() via total <= capacity covered

    if (using_memory_ == false) {
      const int got = ::gzread(file_, buf_.data() + tail_,
                               static_cast<unsigned>(space));
      if (got < 0) {
        int err = 0;
        const char* msg = ::gzerror(file_, &err);
        throw std::runtime_error(std::string("gzread failed: ") +
                                 (msg != nullptr ? msg : "unknown"));
      }
      if (got == 0) {
        eof_ = true;
        return false;
      }
      tail_ += static_cast<std::size_t>(got);
      return true;
    }
    else {
      const std::size_t rest = mem_source_.size() - mem_pos_;
      const std::size_t to_copy = std::min(space, rest);
      std::memcpy(buf_.data() + tail_, mem_source_.data() + mem_pos_, to_copy);
      mem_pos_ += to_copy;
      const int got = static_cast<int>(to_copy);

      if (got == 0) {
        eof_ = true;
        return false;
      }
      tail_ += static_cast<std::size_t>(got);
      return true;
    }
  }

  gzFile file_ = nullptr;
  std::vector<std::byte> buf_;
  std::size_t head_ = 0;  // begin of next message
  std::size_t tail_ = 0;  // end of valid data
  std::uint64_t consumed_ = 0;
  bool eof_ = false;

  std::vector<std::byte> mem_source_;
  std::size_t mem_pos_ = 0;
  bool using_memory_ = false;
};

}  // namespace df