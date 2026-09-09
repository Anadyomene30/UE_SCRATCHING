#include "app/analysis_queue.h"

#include <algorithm>
#include <utility>

namespace svj {

AnalysisQueue::AnalysisQueue(Runner runner) : runner_(std::move(runner)) {
    if (!runner_) {
        runner_ = [](const AnalyzeOptions& options, AnalyzeResult& result,
                     const AnalyzeProgress& progress, std::string& error) {
            return analyze_clip(options, result, progress, error);
        };
    }
    worker_ = std::thread([this] { run(); });
}

AnalysisQueue::~AnalysisQueue() {
    {
        std::lock_guard<std::mutex> guard(lock_);
        stop_ = true;
        jobs_.clear();
    }
    wake_.notify_all();
    if (worker_.joinable()) worker_.join();
}

void AnalysisQueue::enqueue(ClipId clip, AnalyzeOptions options) {
    {
        std::lock_guard<std::mutex> guard(lock_);
        jobs_.push_back(Job{clip, std::move(options)});
    }
    wake_.notify_one();
}

void AnalysisQueue::poll(std::vector<AnalysisReport>& out) {
    std::lock_guard<std::mutex> guard(lock_);
    out.swap(reports_);
    reports_.clear();
}

std::size_t AnalysisQueue::pending() const {
    std::lock_guard<std::mutex> guard(lock_);
    return jobs_.size();
}

bool AnalysisQueue::busy() const {
    std::lock_guard<std::mutex> guard(lock_);
    return busy_;
}

void AnalysisQueue::report(AnalysisReport r) {
    std::lock_guard<std::mutex> guard(lock_);
    // Progress reports for the same clip collapse: the front end only wants
    // the latest figure, and a long pass polled once a frame must not pile up
    // thousands of stale ones behind it.
    if (r.state == AnalysisState::Analysing && !reports_.empty() &&
        reports_.back().clip == r.clip && reports_.back().state == AnalysisState::Analysing) {
        reports_.back() = std::move(r);
        return;
    }
    reports_.push_back(std::move(r));
}

void AnalysisQueue::run() {
    for (;;) {
        Job job;
        {
            std::unique_lock<std::mutex> guard(lock_);
            wake_.wait(guard, [this] { return stop_ || !jobs_.empty(); });
            if (stop_) return;
            job = std::move(jobs_.front());
            jobs_.pop_front();
            busy_ = true;
        }

        AnalysisReport started;
        started.clip = job.clip;
        started.state = AnalysisState::Analysing;
        report(started);

        AnalyzeResult result;
        std::string error;
        const ClipId clip = job.clip;
        const bool ok = runner_(
            job.options, result,
            [this, clip](std::uint32_t done, std::uint32_t estimated) {
                AnalysisReport r;
                r.clip = clip;
                r.state = AnalysisState::Analysing;
                // Capped short of 1: the estimate is an estimate, and a bar
                // that reaches full and then keeps going reads as broken.
                r.progress = estimated > 0
                                 ? std::min(0.99f, static_cast<float>(done) /
                                                       static_cast<float>(estimated))
                                 : 0.0f;
                report(r);
            },
            error);

        AnalysisReport done;
        done.clip = clip;
        done.state = ok ? AnalysisState::Ready : AnalysisState::Failed;
        done.progress = ok ? 1.0f : 0.0f;
        done.result = result;
        done.error = error;
        report(done);

        std::lock_guard<std::mutex> guard(lock_);
        busy_ = false;
    }
}

}  // namespace svj
