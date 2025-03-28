#pragma once

#include <cstdint>
#include <string>
#include <functional>

namespace dedupe {

class Progress {
public:
    using ProgressCallback = std::function<void(const std::string&, double)>;
    using CancellationCallback = std::function<bool()>;

    Progress(ProgressCallback progress_callback = nullptr,
            CancellationCallback cancellation_callback = nullptr)
        : progress_callback_(std::move(progress_callback))
        , cancellation_callback_(std::move(cancellation_callback))
    {}

    void report(const std::string& message, double progress) {
        if (progress_callback_) {
            progress_callback_(message, progress);
        }
    }

    bool is_cancelled() const {
        return cancellation_callback_ && cancellation_callback_();
    }

private:
    ProgressCallback progress_callback_;
    CancellationCallback cancellation_callback_;
};

} // namespace dedupe 