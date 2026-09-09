// scratchvj — analysis in the background, reported at the frame boundary.
//
// An analysis takes seconds to minutes and runs ffmpeg over a pipe; doing it
// on the render thread would freeze the instrument for exactly that long. So
// one worker thread runs the passes in order, and the ONLY thing that crosses
// back is a small report under a lock, which the front end drains once per
// frame at the same boundary it serves clip loads. Nothing here touches the
// Library, the Engine, bgfx or ImGui: those stay on the main thread, which is
// what keeps the render loop single-threaded in every way that matters.
//
// One worker, not a pool. ffmpeg and the disk are the bottleneck, and two
// analyses side by side during a set would fight each other for both while
// the picture on stage stutters -- a queue that is slower and steady beats one
// that is faster and felt.
//
// The runner is injectable so the queue's own behaviour -- ordering, progress,
// failure, shutdown -- is tested with a fake pass rather than with ffmpeg.
#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "app/analyze.h"
#include "core/library.h"

namespace svj {

struct AnalysisReport {
    ClipId clip = kNoClip;
    AnalysisState state = AnalysisState::Queued;
    float progress = 0.0f;
    AnalyzeResult result;  // filled on Ready
    std::string error;     // filled on Failed
};

class AnalysisQueue {
public:
    using Runner = std::function<bool(const AnalyzeOptions&, AnalyzeResult&,
                                      const AnalyzeProgress&, std::string&)>;

    // A null runner means the real pass, analyze_clip.
    explicit AnalysisQueue(Runner runner = nullptr);
    // Finishes the pass in flight -- a half-written cache would be a file
    // that opens and then tears -- and drops the rest.
    ~AnalysisQueue();

    AnalysisQueue(const AnalysisQueue&) = delete;
    AnalysisQueue& operator=(const AnalysisQueue&) = delete;

    // Adds a job. The first report for it is `Analysing` when it starts.
    void enqueue(ClipId clip, AnalyzeOptions options);

    // Hands over every report since the last call, in order. Main thread.
    void poll(std::vector<AnalysisReport>& out);

    std::size_t pending() const;  // waiting, not counting the one running
    bool busy() const;            // a pass is running right now

private:
    struct Job {
        ClipId clip;
        AnalyzeOptions options;
    };

    void run();
    void report(AnalysisReport r);

    Runner runner_;
    mutable std::mutex lock_;
    std::condition_variable wake_;
    std::deque<Job> jobs_;
    std::vector<AnalysisReport> reports_;
    bool busy_ = false;
    bool stop_ = false;
    std::thread worker_;
};

}  // namespace svj
