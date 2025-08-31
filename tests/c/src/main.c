#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "application.h"
#include "database/connection.h"
#include "services/user_service.h"
#include "services/notification_service.h"
#include "utils/logger.h"
#include "utils/config.h"
#include "utils/memory_pool.h"

// Global application state
static Application* g_app = NULL;
static volatile int g_shutdown_requested = 0;

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    switch (signal) {
        case SIGINT:
        case SIGTERM:
            logger_info("Received shutdown signal %d", signal);
            g_shutdown_requested = 1;
            break;
        default:
            logger_warn("Received unhandled signal %d", signal);
            break;
    }
}

// Setup signal handlers
int setup_signals(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        logger_error("Failed to setup SIGINT handler: %s", strerror(errno));
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        logger_error("Failed to setup SIGTERM handler: %s", strerror(errno));
        return -1;
    }

    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    return 0;
}

// Initialize application components
int initialize_application(Application* app, const Config* config) {
    logger_info("Initializing application components");

    // Initialize memory pool
    app->memory_pool = memory_pool_create(1024 * 1024); // 1MB pool
    if (!app->memory_pool) {
        logger_error("Failed to create memory pool");
        return -1;
    }

    // Initialize database connection
    app->database = database_connection_create(
        config->database_host,
        config->database_port,
        config->database_name,
        config->database_user,
        config->database_password
    );
    
    if (!app->database) {
        logger_error("Failed to create database connection");
        return -1;
    }

    if (database_connection_connect(app->database) != 0) {
        logger_error("Failed to connect to database");
        return -1;
    }

    // Initialize user service
    app->user_service = user_service_create(app->database, app->memory_pool);
    if (!app->user_service) {
        logger_error("Failed to create user service");
        return -1;
    }

    // Initialize notification service
    app->notification_service = notification_service_create(
        config->notification_config,
        app->memory_pool
    );
    if (!app->notification_service) {
        logger_error("Failed to create notification service");
        return -1;
    }

    // Initialize services
    if (user_service_initialize(app->user_service) != 0) {
        logger_error("Failed to initialize user service");
        return -1;
    }

    if (notification_service_initialize(app->notification_service) != 0) {
        logger_error("Failed to initialize notification service");
        return -1;
    }

    logger_info("Application components initialized successfully");
    return 0;
}

// Demonstrate complex dependency relationships
int run_sample_workflow(Application* app) {
    logger_info("Starting sample workflow");

    // Create sample users with different roles
    UserCreateRequest admin_request = {
        .email = "admin@example.com",
        .username = "admin_user",
        .first_name = "Admin",
        .last_name = "User",
        .role = USER_ROLE_ADMIN,
        .password = "secure_admin_password"
    };

    UserCreateRequest regular_request = {
        .email = "user@example.com", 
        .username = "regular_user",
        .first_name = "Regular",
        .last_name = "User",
        .role = USER_ROLE_USER,
        .password = "secure_user_password"
    };

    // Create users through service
    User* admin_user = NULL;
    User* regular_user = NULL;

    if (user_service_create_user(app->user_service, &admin_request, &admin_user) != 0) {
        logger_error("Failed to create admin user");
        return -1;
    }

    if (user_service_create_user(app->user_service, &regular_request, &regular_user) != 0) {
        logger_error("Failed to create regular user");
        user_free(admin_user);
        return -1;
    }

    logger_info("Created admin user with ID: %lu", admin_user->id);
    logger_info("Created regular user with ID: %lu", regular_user->id);

    // Demonstrate service interactions
    UserList* active_users = NULL;
    if (user_service_get_active_users(app->user_service, &active_users) == 0) {
        logger_info("Found %zu active users", active_users->count);

        // Send notifications to admin users
        for (size_t i = 0; i < active_users->count; i++) {
            User* user = active_users->users[i];
            if (user->role == USER_ROLE_ADMIN) {
                NotificationRequest notification = {
                    .user_id = user->id,
                    .type = NOTIFICATION_WELCOME,
                    .title = "Welcome Admin",
                    .message = "You have been granted admin privileges",
                    .priority = NOTIFICATION_PRIORITY_HIGH
                };

                if (notification_service_send(app->notification_service, &notification) != 0) {
                    logger_warn("Failed to send notification to user %lu", user->id);
                }
            }
        }

        user_list_free(active_users);
    }

    // Demonstrate caching and retrieval
    User* cached_user = NULL;
    if (user_service_get_by_id(app->user_service, admin_user->id, &cached_user) == 0) {
        logger_info("Retrieved user from cache: %s", cached_user->username);
        user_free(cached_user);
    }

    // Update user information
    UserUpdateRequest update_request = {
        .user_id = regular_user->id,
        .first_name = "Updated",
        .last_name = "Name",
        .role = USER_ROLE_MODERATOR
    };

    if (user_service_update_user(app->user_service, &update_request) == 0) {
        logger_info("Successfully updated user %lu", regular_user->id);
    }

    // Cleanup
    user_free(admin_user);
    user_free(regular_user);

    logger_info("Sample workflow completed successfully");
    return 0;
}

// Main application loop
int run_application(Application* app) {
    logger_info("Starting main application loop");

    // Run the sample workflow
    if (run_sample_workflow(app) != 0) {
        logger_error("Sample workflow failed");
        return -1;
    }

    // Main event loop
    while (!g_shutdown_requested) {
        // Process any pending notifications
        notification_service_process_pending(app->notification_service);

        // Perform periodic maintenance
        user_service_cleanup_expired_sessions(app->user_service);

        // Database connection health check
        if (database_connection_ping(app->database) != 0) {
            logger_warn("Database connection health check failed");
            
            // Attempt to reconnect
            if (database_connection_reconnect(app->database) != 0) {
                logger_error("Failed to reconnect to database");
                break;
            }
        }

        // Sleep briefly to prevent busy waiting
        usleep(100000); // 100ms
    }

    logger_info("Application loop exiting");
    return 0;
}

// Cleanup application resources
void cleanup_application(Application* app) {
    if (!app) return;

    logger_info("Cleaning up application resources");

    // Shutdown services in reverse dependency order
    if (app->notification_service) {
        notification_service_shutdown(app->notification_service);
        notification_service_destroy(app->notification_service);
        app->notification_service = NULL;
    }

    if (app->user_service) {
        user_service_shutdown(app->user_service);
        user_service_destroy(app->user_service);
        app->user_service = NULL;
    }

    if (app->database) {
        database_connection_disconnect(app->database);
        database_connection_destroy(app->database);
        app->database = NULL;
    }

    if (app->memory_pool) {
        memory_pool_destroy(app->memory_pool);
        app->memory_pool = NULL;
    }

    logger_info("Application cleanup completed");
}

int main(int argc, char* argv[]) {
    int exit_code = EXIT_SUCCESS;

    printf("Starting Crawler Test C Application\n");

    // Initialize logging
    if (logger_initialize(LOG_LEVEL_INFO, "crawler_test.log") != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }

    logger_info("Application starting with PID %d", getpid());

    // Load configuration
    Config* config = config_load_from_env();
    if (!config) {
        logger_error("Failed to load configuration");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    // Setup signal handlers
    if (setup_signals() != 0) {
        logger_error("Failed to setup signal handlers");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    // Create application instance
    Application app = {0};
    g_app = &app;

    // Initialize application
    if (initialize_application(&app, config) != 0) {
        logger_error("Failed to initialize application");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    // Run main application logic
    if (run_application(&app) != 0) {
        logger_error("Application execution failed");
        exit_code = EXIT_FAILURE;
    }

cleanup:
    // Cleanup resources
    cleanup_application(&app);
    
    if (config) {
        config_free(config);
    }

    logger_info("Application exiting with code %d", exit_code);
    logger_shutdown();

    printf("Crawler Test C Application completed\n");
    return exit_code;
}