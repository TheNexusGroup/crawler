<?php

declare(strict_types=1);

namespace CrawlerTest;

use CrawlerTest\Service\UserService;
use CrawlerTest\Service\NotificationService;
use CrawlerTest\Service\CacheService;
use CrawlerTest\Repository\UserRepository;
use CrawlerTest\Entity\User;
use CrawlerTest\Utils\Logger;
use CrawlerTest\Config\DatabaseConfig;
use Doctrine\ORM\EntityManagerInterface;
use Psr\Log\LoggerInterface;

/**
 * Main application class demonstrating complex PHP dependency relationships.
 * 
 * This application showcases various patterns including:
 * - Dependency injection
 * - Service layer architecture  
 * - Repository pattern with Doctrine ORM
 * - Interface-based design
 * - Namespace organization
 * - Trait usage for shared functionality
 */
class Application
{
    private UserService $userService;
    private NotificationService $notificationService;
    private CacheService $cacheService;
    private LoggerInterface $logger;
    private EntityManagerInterface $entityManager;

    public function __construct(
        UserService $userService,
        NotificationService $notificationService,
        CacheService $cacheService,
        LoggerInterface $logger,
        EntityManagerInterface $entityManager
    ) {
        $this->userService = $userService;
        $this->notificationService = $notificationService;
        $this->cacheService = $cacheService;
        $this->logger = $logger;
        $this->entityManager = $entityManager;
    }

    /**
     * Initialize the application and all its dependencies.
     */
    public function initialize(): void
    {
        $this->logger->info('Initializing Crawler Test PHP Application');

        try {
            // Initialize database connection
            $this->entityManager->getConnection()->connect();
            $this->logger->info('Database connection established');

            // Initialize cache service
            $this->cacheService->connect();
            $this->logger->info('Cache service initialized');

            // Initialize services
            $this->userService->initialize();
            $this->notificationService->initialize();

            $this->logger->info('All services initialized successfully');

        } catch (\Exception $e) {
            $this->logger->error('Failed to initialize application', [
                'error' => $e->getMessage(),
                'trace' => $e->getTraceAsString()
            ]);
            throw $e;
        }
    }

    /**
     * Run the main application workflow.
     */
    public function run(): void
    {
        $this->logger->info('Starting main application workflow');

        try {
            $this->executeSampleWorkflow();
            $this->logger->info('Main workflow completed successfully');

        } catch (\Exception $e) {
            $this->logger->error('Main workflow failed', [
                'error' => $e->getMessage()
            ]);
            throw $e;
        }
    }

    /**
     * Execute a sample workflow demonstrating service dependencies.
     */
    private function executeSampleWorkflow(): void
    {
        $this->logger->info('Starting sample workflow execution');

        // Create sample users with different roles
        $adminUser = $this->createSampleUser(
            'admin@example.com',
            'admin_user',
            'Admin',
            'User',
            User::ROLE_ADMIN
        );

        $moderatorUser = $this->createSampleUser(
            'moderator@example.com',
            'mod_user',
            'Moderator',
            'User',
            User::ROLE_MODERATOR
        );

        $regularUser = $this->createSampleUser(
            'user@example.com',
            'regular_user',
            'Regular',
            'User',
            User::ROLE_USER
        );

        $this->logger->info('Created sample users', [
            'admin_id' => $adminUser->getId(),
            'moderator_id' => $moderatorUser->getId(),
            'user_id' => $regularUser->getId()
        ]);

        // Demonstrate service interactions
        $this->demonstrateUserOperations();
        $this->demonstrateServiceInteractions($adminUser, $moderatorUser, $regularUser);
        $this->demonstrateCachingOperations();

        $this->logger->info('Sample workflow completed');
    }

    /**
     * Create a sample user with specified details.
     */
    private function createSampleUser(
        string $email,
        string $username,
        string $firstName,
        string $lastName,
        string $role
    ): User {
        $userData = [
            'email' => $email,
            'username' => $username,
            'firstName' => $firstName,
            'lastName' => $lastName,
            'role' => $role,
            'status' => User::STATUS_ACTIVE,
            'emailVerified' => true
        ];

        return $this->userService->createUser($userData);
    }

    /**
     * Demonstrate various user service operations.
     */
    private function demonstrateUserOperations(): void
    {
        $this->logger->info('Demonstrating user service operations');

        // Get active users
        $activeUsers = $this->userService->getActiveUsers();
        $this->logger->info('Found active users', ['count' => count($activeUsers)]);

        // Get users by role
        $adminUsers = $this->userService->getUsersByRole(User::ROLE_ADMIN);
        $this->logger->info('Found admin users', ['count' => count($adminUsers)]);

        // Demonstrate user search
        $searchResults = $this->userService->searchUsers('admin');
        $this->logger->info('User search results', ['count' => count($searchResults)]);

        // Get user statistics
        $stats = $this->userService->getUserStatistics();
        $this->logger->info('User statistics', $stats);
    }

    /**
     * Demonstrate interactions between different services.
     */
    private function demonstrateServiceInteractions(User $adminUser, User $moderatorUser, User $regularUser): void
    {
        $this->logger->info('Demonstrating service interactions');

        // Send welcome notifications
        $this->notificationService->sendWelcomeNotification($regularUser);
        $this->notificationService->sendWelcomeNotification($moderatorUser);
        $this->notificationService->sendAdminWelcomeNotification($adminUser);

        // Demonstrate user role promotion
        $this->userService->promoteUser($regularUser->getId(), User::ROLE_MODERATOR);
        $this->logger->info('Promoted user to moderator', [
            'user_id' => $regularUser->getId()
        ]);

        // Send bulk notifications
        $userIds = [
            $adminUser->getId(),
            $moderatorUser->getId(),
            $regularUser->getId()
        ];

        $this->userService->sendBulkNotification(
            $userIds,
            'System maintenance scheduled for tonight'
        );

        $this->logger->info('Sent bulk notification', [
            'recipient_count' => count($userIds)
        ]);
    }

    /**
     * Demonstrate caching operations and patterns.
     */
    private function demonstrateCachingOperations(): void
    {
        $this->logger->info('Demonstrating caching operations');

        // Cache user data
        $users = $this->userService->getActiveUsers();
        $cacheKey = 'active_users_list';
        
        $this->cacheService->set($cacheKey, $users, 3600); // Cache for 1 hour
        $this->logger->info('Cached active users list');

        // Retrieve from cache
        $cachedUsers = $this->cacheService->get($cacheKey);
        if ($cachedUsers !== null) {
            $this->logger->info('Retrieved users from cache', [
                'count' => count($cachedUsers)
            ]);
        }

        // Demonstrate cache invalidation
        $this->cacheService->delete($cacheKey);
        $this->logger->info('Invalidated cache key', ['key' => $cacheKey]);

        // Demonstrate cache tags
        $this->cacheService->setWithTags(
            'user_stats',
            $this->userService->getUserStatistics(),
            ['users', 'statistics'],
            7200 // 2 hours
        );

        // Invalidate by tags
        $this->cacheService->invalidateByTags(['users']);
        $this->logger->info('Invalidated cache by tags', ['tags' => ['users']]);
    }

    /**
     * Graceful shutdown of the application.
     */
    public function shutdown(): void
    {
        $this->logger->info('Starting application shutdown');

        try {
            // Shutdown services in reverse dependency order
            $this->notificationService->shutdown();
            $this->userService->shutdown();
            $this->cacheService->disconnect();
            $this->entityManager->getConnection()->close();

            $this->logger->info('Application shutdown completed successfully');

        } catch (\Exception $e) {
            $this->logger->error('Error during application shutdown', [
                'error' => $e->getMessage()
            ]);
        }
    }
}