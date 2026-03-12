#include "sync_engine.h"
#include <spdlog/spdlog.h>

namespace hydra::gcode {

SyncEngine::SyncEngine(Stream& stream, int nozzle_count)
    : stream_(stream), nozzle_count_(nozzle_count) {}

std::optional<SyncEngine::TaggedCommand> SyncEngine::next() {
    /* Try to advance any running nozzle */
    for (int i = 0; i < nozzle_count_; i++) {
        if (ctx_[i].state == NozzleState::Running) {
            try_advance(i);
        }
    }

    /* Try to release blocked nozzles */
    try_release_barriers();
    try_release_tasks();

    /* Yield a command from any running nozzle that has a non-sync command */
    for (int i = 0; i < nozzle_count_; i++) {
        if (ctx_[i].state == NozzleState::Running && ctx_[i].pending) {
            auto cmd = std::move(*ctx_[i].pending);
            ctx_[i].pending.reset();
            return TaggedCommand{i, std::move(cmd)};
        }
    }

    return std::nullopt;
}

bool SyncEngine::done() const {
    for (int i = 0; i < nozzle_count_; i++) {
        if (ctx_[i].state != NozzleState::Done) return false;
    }
    return true;
}

void SyncEngine::try_advance(int nozzle) {
    auto& c = ctx_[nozzle];
    if (c.pending) return; /* Already have a buffered command */

    auto cmd = stream_.next(nozzle);
    if (!cmd) {
        c.state = NozzleState::Done;
        return;
    }

    /* Handle sync markers — block the nozzle */
    if (auto* barrier = std::get_if<SyncBarrier>(&*cmd)) {
        c.state = NozzleState::BlockedOnBarrier;
        c.barrier_id = barrier->id;
        spdlog::debug("Nozzle {} blocked on barrier '{}'", nozzle, barrier->id);
        return;
    }

    if (auto* wait = std::get_if<WaitTask>(&*cmd)) {
        if (completed_tasks_.contains(wait->task_id)) {
            /* Dependency already met — skip the wait */
            spdlog::debug("Nozzle {} WAIT TASK {} already completed", nozzle, wait->task_id);
            /* Read next command instead */
            try_advance(nozzle);
            return;
        }
        c.state = NozzleState::BlockedOnTask;
        c.wait_task_id = wait->task_id;
        spdlog::debug("Nozzle {} blocked on task {}", nozzle, wait->task_id);
        return;
    }

    if (auto* begin = std::get_if<TaskBegin>(&*cmd)) {
        spdlog::debug("Nozzle {} TASK BEGIN {}", nozzle, begin->task_id);
        /* Task begin is informational — continue reading */
        try_advance(nozzle);
        return;
    }

    if (auto* end = std::get_if<TaskEnd>(&*cmd)) {
        completed_tasks_.insert(end->task_id);
        spdlog::debug("Nozzle {} TASK END {} (completed)", nozzle, end->task_id);
        /* Task end is informational — continue reading */
        try_advance(nozzle);
        return;
    }

    /* Regular command — buffer it for yielding */
    c.pending = std::move(cmd);
}

bool SyncEngine::try_release_barriers() {
    /* Check if all blocked nozzles share the same barrier ID */
    bool any_released = false;

    /* Find the barrier ID that blocked nozzles are waiting on */
    std::string barrier_id;
    int blocked_count = 0;
    int eligible_count = 0; /* Nozzles that must participate */

    for (int i = 0; i < nozzle_count_; i++) {
        if (ctx_[i].state == NozzleState::Done) continue;
        eligible_count++;

        if (ctx_[i].state == NozzleState::BlockedOnBarrier) {
            if (barrier_id.empty()) {
                barrier_id = ctx_[i].barrier_id;
            }
            if (ctx_[i].barrier_id == barrier_id) {
                blocked_count++;
            }
        }
    }

    /* Release if all eligible nozzles have reached the same barrier */
    if (!barrier_id.empty() && blocked_count == eligible_count) {
        spdlog::debug("Barrier '{}' released ({} nozzles)", barrier_id, blocked_count);
        for (int i = 0; i < nozzle_count_; i++) {
            if (ctx_[i].state == NozzleState::BlockedOnBarrier &&
                ctx_[i].barrier_id == barrier_id) {
                ctx_[i].state = NozzleState::Running;
                ctx_[i].barrier_id.clear();
                try_advance(i);
            }
        }
        any_released = true;
    }

    return any_released;
}

bool SyncEngine::try_release_tasks() {
    bool any_released = false;

    for (int i = 0; i < nozzle_count_; i++) {
        if (ctx_[i].state == NozzleState::BlockedOnTask) {
            if (completed_tasks_.contains(ctx_[i].wait_task_id)) {
                spdlog::debug("Nozzle {} released (task {} completed)",
                             i, ctx_[i].wait_task_id);
                ctx_[i].state = NozzleState::Running;
                ctx_[i].wait_task_id = 0;
                try_advance(i);
                any_released = true;
            }
        }
    }

    return any_released;
}

} // namespace hydra::gcode
