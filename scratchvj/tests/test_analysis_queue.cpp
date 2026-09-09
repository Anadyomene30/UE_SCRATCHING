#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "app/analysis_queue.h"
#include "harness.h"

using namespace svj;

namespace {

// Drains the queue until `until` says stop or a generous deadline passes.
template <typename Pred>
std::vector<AnalysisReport> drain_until(AnalysisQueue& queue, Pred until) {
    std::vector<AnalysisReport> all, batch;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        queue.poll(batch);
        all.insert(all.end(), batch.begin(), batch.end());
        if (until(all)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return all;
}

bool has_state(const std::vector<AnalysisReport>& reports, ClipId clip, AnalysisState state) {
    for (const AnalysisReport& r : reports) {
        if (r.clip == clip && r.state == state) return true;
    }
    return false;
}

}  // namespace

SVJ_TEST("analysis_queue: jobs run one after another, in the order they were queued") {
    std::vector<std::string> order;
    std::mutex order_lock;
    AnalysisQueue queue([&](const AnalyzeOptions& o, AnalyzeResult& r, const AnalyzeProgress&,
                            std::string&) {
        std::lock_guard<std::mutex> guard(order_lock);
        order.push_back(o.input);
        r.frames = 1;
        return true;
    });
    for (int i = 0; i < 4; ++i) {
        AnalyzeOptions options;
        options.input = "clip" + std::to_string(i);
        queue.enqueue(static_cast<ClipId>(i), options);
    }
    const auto reports = drain_until(queue, [](const std::vector<AnalysisReport>& all) {
        return has_state(all, 3, AnalysisState::Ready);
    });
    CHECK_EQ(order.size(), std::size_t{4});
    CHECK_EQ(order[0], std::string("clip0"));
    CHECK_EQ(order[3], std::string("clip3"));
    for (int i = 0; i < 4; ++i) CHECK(has_state(reports, i, AnalysisState::Ready));
}

SVJ_TEST("analysis_queue: progress arrives before Ready, as a fraction under one") {
    AnalysisQueue queue([&](const AnalyzeOptions&, AnalyzeResult& r, const AnalyzeProgress& p,
                            std::string&) {
        p(30, 100);
        p(60, 100);
        r.frames = 100;
        return true;
    });
    queue.enqueue(7, AnalyzeOptions{});
    const auto reports = drain_until(queue, [](const std::vector<AnalysisReport>& all) {
        return has_state(all, 7, AnalysisState::Ready);
    });
    bool saw_progress = false;
    bool ready_after = false;
    for (const AnalysisReport& r : reports) {
        if (r.state == AnalysisState::Analysing && r.progress > 0.0f) {
            saw_progress = true;
            CHECK(r.progress < 1.0f);
        }
        if (r.state == AnalysisState::Ready) {
            ready_after = saw_progress;
            CHECK_EQ(r.result.frames, 100u);
        }
    }
    CHECK(saw_progress);
    CHECK(ready_after);
}

SVJ_TEST("analysis_queue: an unknown total reports zero progress rather than a guess") {
    AnalysisQueue queue([&](const AnalyzeOptions&, AnalyzeResult&, const AnalyzeProgress& p,
                            std::string&) {
        p(30, 0);
        return true;
    });
    queue.enqueue(1, AnalyzeOptions{});
    const auto reports = drain_until(queue, [](const std::vector<AnalysisReport>& all) {
        return has_state(all, 1, AnalysisState::Ready);
    });
    for (const AnalysisReport& r : reports) {
        if (r.state == AnalysisState::Analysing) CHECK_NEAR(r.progress, 0.0, 1e-9);
    }
}

SVJ_TEST("analysis_queue: a failing pass reports Failed with its own message") {
    AnalysisQueue queue([&](const AnalyzeOptions&, AnalyzeResult&, const AnalyzeProgress&,
                            std::string& error) {
        error = "ffmpeg introuvable";
        return false;
    });
    queue.enqueue(2, AnalyzeOptions{});
    const auto reports = drain_until(queue, [](const std::vector<AnalysisReport>& all) {
        return has_state(all, 2, AnalysisState::Failed);
    });
    bool found = false;
    for (const AnalysisReport& r : reports) {
        if (r.state == AnalysisState::Failed) {
            found = true;
            CHECK_EQ(r.error, std::string("ffmpeg introuvable"));
        }
    }
    CHECK(found);
    CHECK(!has_state(reports, 2, AnalysisState::Ready));
}

SVJ_TEST("analysis_queue: destroying the queue mid-job finishes that job and drops the rest") {
    // A half-written cache is a file that opens and then tears, so the pass
    // in flight is allowed to finish; what has not started never does.
    std::atomic<int> started{0};
    {
        AnalysisQueue queue([&](const AnalyzeOptions&, AnalyzeResult&, const AnalyzeProgress&,
                                std::string&) {
            ++started;
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            return true;
        });
        for (int i = 0; i < 5; ++i) queue.enqueue(i, AnalyzeOptions{});
        // Give the worker a moment to take the first job, then leave scope.
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    CHECK(started.load() >= 1);
    CHECK(started.load() < 5);
}

SVJ_TEST("analysis_queue: polling an idle queue returns nothing and reports not busy") {
    AnalysisQueue queue([](const AnalyzeOptions&, AnalyzeResult&, const AnalyzeProgress&,
                           std::string&) { return true; });
    std::vector<AnalysisReport> reports;
    queue.poll(reports);
    CHECK(reports.empty());
    CHECK(!queue.busy());
    CHECK_EQ(queue.pending(), std::size_t{0});
}
