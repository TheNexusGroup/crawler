#ifndef PARALLEL_PROCESSOR_H
#define PARALLEL_PROCESSOR_H

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>
#include "dependency.h"
#include "memory_pool.h"

// Configuration for parallel processing
#define MAX_WORKER_THREADS 16
#define DEFAULT_THREAD_COUNT 4
#define WORK_QUEUE_SIZE 1024
#define BATCH_SIZE 32

// Work item structure for the thread pool
typedef struct WorkItem {
    char* file_path;
    char* file_content;
    size_t content_size;
    LanguageType language;
    struct WorkItem* next;
} WorkItem;

// Thread-safe work queue
typedef struct WorkQueue {
    WorkItem* head;
    WorkItem* tail;
    size_t count;
    size_t max_size;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    bool shutdown;
} WorkQueue;

// Worker thread data
typedef struct WorkerThread {
    pthread_t thread;
    int thread_id;
    struct ParallelProcessor* processor;
    bool active;
    size_t files_processed;
    size_t total_processing_time_ms;
} WorkerThread;

// Result collection structure
typedef struct ProcessingResult {
    char* file_path;
    ExtractedDependency* dependencies;
    AnalysisLayer layers_analyzed;
    size_t processing_time_ms;
    bool success;
    char* error_message;
    struct ProcessingResult* next;
} ProcessingResult;

// Thread-safe result queue
typedef struct ResultQueue {
    ProcessingResult* head;
    ProcessingResult* tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} ResultQueue;

// Main parallel processor structure
typedef struct ParallelProcessor {
    WorkerThread* workers;
    int thread_count;
    WorkQueue* work_queue;
    ResultQueue* result_queue;
    MemoryPool* memory_pool;
    
    // Statistics
    size_t total_files_queued;
    size_t total_files_processed;
    size_t total_files_failed;
    size_t total_processing_time_ms;
    
    // Configuration
    bool use_memory_mapping;
    bool enable_caching;
    size_t max_file_size;
    
    // Synchronization
    pthread_mutex_t stats_mutex;
    bool shutdown_requested;
    bool initialized;
} ParallelProcessor;

// Processor statistics
typedef struct ProcessorStats {
    size_t files_queued;
    size_t files_processed;
    size_t files_failed;
    size_t average_processing_time_ms;
    size_t thread_utilization_percent;
    size_t memory_usage_bytes;
} ProcessorStats;

// Parallel processor functions
ParallelProcessor* parallel_processor_create(int thread_count, size_t memory_pool_size);
void parallel_processor_destroy(ParallelProcessor* processor);

// Work queue operations
int parallel_processor_queue_file(ParallelProcessor* processor, const char* file_path);
int parallel_processor_queue_batch(ParallelProcessor* processor, char** file_paths, size_t count);
int parallel_processor_queue_directory(ParallelProcessor* processor, const char* directory);

// Result collection
ProcessingResult* parallel_processor_get_result(ParallelProcessor* processor);
ProcessingResult* parallel_processor_get_all_results(ParallelProcessor* processor);
void processing_result_free(ProcessingResult* result);

// Processing control
int parallel_processor_start(ParallelProcessor* processor);
int parallel_processor_stop(ParallelProcessor* processor);
int parallel_processor_wait_completion(ParallelProcessor* processor, int timeout_ms);
void parallel_processor_shutdown(ParallelProcessor* processor);

// Configuration
void parallel_processor_set_memory_mapping(ParallelProcessor* processor, bool enabled);
void parallel_processor_set_caching(ParallelProcessor* processor, bool enabled);
void parallel_processor_set_max_file_size(ParallelProcessor* processor, size_t max_size);

// Statistics and monitoring
ProcessorStats parallel_processor_get_stats(const ParallelProcessor* processor);
void parallel_processor_debug_print(const ParallelProcessor* processor);
float parallel_processor_get_progress(const ParallelProcessor* processor);

// Thread pool management
int parallel_processor_resize_pool(ParallelProcessor* processor, int new_thread_count);
void parallel_processor_pause(ParallelProcessor* processor);
void parallel_processor_resume(ParallelProcessor* processor);

#endif // PARALLEL_PROCESSOR_H