#ifndef APPLICATION_H
#define APPLICATION_H

#include "database/connection.h"
#include "services/user_service.h"
#include "services/notification_service.h"
#include "utils/memory_pool.h"

/**
 * Main application structure containing all service dependencies
 */
typedef struct Application {
    DatabaseConnection* database;
    UserService* user_service;
    NotificationService* notification_service;
    MemoryPool* memory_pool;
} Application;

#endif // APPLICATION_H