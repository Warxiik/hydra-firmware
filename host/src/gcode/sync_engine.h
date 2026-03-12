#pragma once

#include "command.h"
#include "stream.h"
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <string>

namespace hydra::gcode {

/**
 * Dual-nozzle synchronization engine.
 *
 * Coordinates two G-code streams using the HydraSlicer sync protocol:
 *
 * Barrier mode:  Both nozzles must reach the same SYNC BARRIER ID
 *                before either proceeds past it.
 *
 * Task mode:     Nozzles run independently. WAIT TASK blocks a nozzle
 *                until the referenced task (on another nozzle) completes.
 *                TASK BEGIN/END bracket task execution.
 *
 * The engine drains commands from both streams and yields them in a
 * coordinated order that respects synchronization constraints.
 */
class SyncEngine {
public:
    explicit SyncEngine(Stream& stream, int nozzle_count = 2);

    /**
     * Get the next executable command for any nozzle.
     * Returns the nozzle ID and command, or nullopt if both streams
     * are blocked on sync or EOF.
     */
    struct TaggedCommand {
        int nozzle_id;
        Command command;
    };
    std::optional<TaggedCommand> next();

    /* Is everything done? */
    bool done() const;

private:
    enum class NozzleState {
        Running,           /* Actively reading commands */
        BlockedOnBarrier,  /* Waiting for other nozzle to reach barrier */
        BlockedOnTask,     /* Waiting for a task dependency */
        Done,              /* Stream EOF */
    };

    void try_advance(int nozzle);
    bool try_release_barriers();
    bool try_release_tasks();

    Stream& stream_;
    int nozzle_count_;

    struct NozzleContext {
        NozzleState state = NozzleState::Running;
        std::optional<Command> pending; /* Buffered command (sync marker) */
        std::string barrier_id;         /* Barrier we're waiting on */
        uint32_t wait_task_id = 0;      /* Task we're waiting for */
    };
    NozzleContext ctx_[2];

    /* Track completed tasks */
    std::unordered_set<uint32_t> completed_tasks_;
};

} // namespace hydra::gcode
