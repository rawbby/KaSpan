#include <debug/assert.hpp>
#include <debug/sub_process.hpp>
#include <scc/base.hpp>
#include <briefkasten/buffered_queue.hpp>
#include <briefkasten/queue_builder.hpp>
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>

int main(int argc, char** argv) {
    mpi_sub_process(argc, argv);
    KASPAN_DEFAULT_INIT();

    int const rank = mpi_world_rank;
    int const size = mpi_world_size;

    auto mq = briefkasten::BufferedMessageQueueBuilder<int>().build();

    std::vector<int> received;
    auto on_message = [&](auto env) {
        for (auto val : env.message) {
            received.push_back(val);
        }
    };

    // Each rank sends its rank to all other ranks
    for (int i = 0; i < size; ++i) {
        if (i != rank) {
            mq.post_message_blocking(rank, i, on_message);
        }
    }

    // Process local messages and wait for termination
    do {
        mq.poll_throttled(on_message);
    } while (!mq.terminate(on_message));

    // Check if we received messages from all other ranks
    ASSERT_EQ(received.size(), static_cast<size_t>(size - 1));
    std::vector<int> expected_phase1(size);
    std::iota(expected_phase1.begin(), expected_phase1.end(), 0);
    expected_phase1.erase(expected_phase1.begin() + rank);
    std::sort(received.begin(), received.end());
    for (size_t i = 0; i < received.size(); ++i) {
        ASSERT_EQ(received[i], expected_phase1[i]);
    }

    // SECOND PHASE
    mq.reactivate(); 
    received.clear();
    for (int i = 0; i < size; ++i) {
        if (i != rank) {
            mq.post_message_blocking(rank + 100, i, on_message);
        }
    }

    do {
        mq.poll_throttled(on_message);
    } while (!mq.terminate(on_message));

    ASSERT_EQ(received.size(), static_cast<size_t>(size - 1));
    std::vector<int> expected_phase2(size);
    for(int i=0; i<size; ++i) expected_phase2[i] = i + 100;
    expected_phase2.erase(expected_phase2.begin() + rank);

    std::sort(received.begin(), received.end());
    for (size_t i = 0; i < received.size(); ++i) {
        ASSERT_EQ(received[i], expected_phase2[i]);
    }

    if (rank == 0) {
        std::cout << "Briefkasten test passed!" << std::endl;
    }

    return 0;
}
