#pragma once
#include <atomic>
inline void DataMemBarrier () { std::atomic_thread_fence (std::memory_order_seq_cst); }
inline void DataSyncBarrier () { std::atomic_thread_fence (std::memory_order_seq_cst); }
