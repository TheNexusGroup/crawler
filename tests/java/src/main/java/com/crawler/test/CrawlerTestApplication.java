package com.crawler.test;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.data.jpa.repository.config.EnableJpaRepositories;
import org.springframework.transaction.annotation.EnableTransactionManagement;

import com.crawler.test.services.UserService;
import com.crawler.test.services.NotificationService;
import com.crawler.test.repositories.UserRepository;
import com.crawler.test.models.User;
import com.crawler.test.config.ApplicationConfig;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.CommandLineRunner;

import java.util.List;

/**
 * Main Spring Boot application demonstrating complex dependency relationships.
 * This application showcases various patterns including:
 * - Service layer architecture
 * - Repository pattern with JPA
 * - Dependency injection
 * - Transaction management
 * - Configuration management
 */
@SpringBootApplication
@ComponentScan(basePackages = "com.crawler.test")
@EnableJpaRepositories(basePackages = "com.crawler.test.repositories")
@EnableTransactionManagement
public class CrawlerTestApplication implements CommandLineRunner {
    
    private static final Logger logger = LoggerFactory.getLogger(CrawlerTestApplication.class);
    
    @Autowired
    private UserService userService;
    
    @Autowired
    private NotificationService notificationService;
    
    @Autowired
    private UserRepository userRepository;
    
    @Autowired
    private ApplicationConfig applicationConfig;

    public static void main(String[] args) {
        logger.info("Starting Crawler Test Java Application");
        
        try {
            SpringApplication app = new SpringApplication(CrawlerTestApplication.class);
            
            // Configure application properties
            app.setAdditionalProfiles("development");
            
            // Run the application
            app.run(args);
            
        } catch (Exception e) {
            logger.error("Failed to start application", e);
            System.exit(1);
        }
    }

    @Override
    public void run(String... args) throws Exception {
        logger.info("Executing sample workflow to demonstrate dependency relationships");
        
        try {
            // Demonstrate complex service interactions
            executeSampleWorkflow();
            
            logger.info("Sample workflow completed successfully");
            
        } catch (Exception e) {
            logger.error("Sample workflow failed", e);
            throw e;
        }
    }
    
    /**
     * Execute a sample workflow that demonstrates various dependency patterns
     * and service interactions within the application.
     */
    private void executeSampleWorkflow() {
        logger.info("Starting sample workflow execution");
        
        // Create sample users with different roles
        User adminUser = createSampleUser(
            "admin@example.com", 
            "admin_user", 
            "Admin", 
            "User", 
            User.Role.ADMIN
        );
        
        User regularUser = createSampleUser(
            "user@example.com", 
            "regular_user", 
            "Regular", 
            "User", 
            User.Role.USER
        );
        
        User moderatorUser = createSampleUser(
            "moderator@example.com", 
            "mod_user", 
            "Moderator", 
            "User", 
            User.Role.MODERATOR
        );
        
        logger.info("Created {} users", 3);
        
        // Demonstrate service layer operations
        demonstrateUserServiceOperations();
        
        // Demonstrate cross-service dependencies
        demonstrateServiceInteractions(adminUser, regularUser, moderatorUser);
        
        // Demonstrate repository operations
        demonstrateRepositoryOperations();
        
        logger.info("Sample workflow execution completed");
    }
    
    /**
     * Create a sample user with the specified details.
     */
    private User createSampleUser(String email, String username, String firstName, String lastName, User.Role role) {
        User user = new User();
        user.setEmail(email);
        user.setUsername(username);
        user.setFirstName(firstName);
        user.setLastName(lastName);
        user.setRole(role);
        user.setStatus(User.Status.ACTIVE);
        user.setEmailVerified(true);
        
        return userService.createUser(user);
    }
    
    /**
     * Demonstrate various user service operations including CRUD operations,
     * validation, caching, and business logic.
     */
    private void demonstrateUserServiceOperations() {
        logger.info("Demonstrating user service operations");
        
        // Get all active users
        List<User> activeUsers = userService.findActiveUsers();
        logger.info("Found {} active users", activeUsers.size());
        
        // Demonstrate user search and filtering
        List<User> adminUsers = userService.findUsersByRole(User.Role.ADMIN);
        logger.info("Found {} admin users", adminUsers.size());
        
        // Demonstrate user validation and business rules
        if (!activeUsers.isEmpty()) {
            User firstUser = activeUsers.get(0);
            boolean canPerformAdminAction = userService.canUserPerformAction(
                firstUser.getId(), 
                "ADMIN_ACTION"
            );
            logger.info("User {} can perform admin action: {}", firstUser.getUsername(), canPerformAdminAction);
        }
        
        // Demonstrate user statistics
        var userStats = userService.getUserStatistics();
        logger.info("User statistics - Total: {}, Active: {}, Inactive: {}", 
            userStats.getTotalUsers(), 
            userStats.getActiveUsers(), 
            userStats.getInactiveUsers()
        );
    }
    
    /**
     * Demonstrate interactions between different services showing how they
     * depend on each other and coordinate to provide business functionality.
     */
    private void demonstrateServiceInteractions(User adminUser, User regularUser, User moderatorUser) {
        logger.info("Demonstrating service interactions and cross-dependencies");
        
        // Send welcome notifications to new users
        try {
            notificationService.sendWelcomeNotification(regularUser);
            notificationService.sendWelcomeNotification(moderatorUser);
            
            // Send special admin notification
            notificationService.sendAdminWelcomeNotification(adminUser);
            
            logger.info("Successfully sent welcome notifications to all users");
            
        } catch (Exception e) {
            logger.error("Failed to send notifications", e);
        }
        
        // Demonstrate user service calling notification service
        try {
            userService.promoteUserToModerator(regularUser.getId());
            logger.info("Successfully promoted user {} to moderator", regularUser.getUsername());
            
        } catch (Exception e) {
            logger.error("Failed to promote user", e);
        }
        
        // Demonstrate batch operations
        List<Long> userIds = List.of(adminUser.getId(), regularUser.getId(), moderatorUser.getId());
        
        try {
            userService.sendBulkNotification(userIds, "System maintenance scheduled for tonight");
            logger.info("Successfully sent bulk notification to {} users", userIds.size());
            
        } catch (Exception e) {
            logger.error("Failed to send bulk notification", e);
        }
    }
    
    /**
     * Demonstrate direct repository operations showing data access patterns
     * and database interactions.
     */
    private void demonstrateRepositoryOperations() {
        logger.info("Demonstrating repository operations and data access patterns");
        
        // Custom repository queries
        List<User> recentUsers = userRepository.findUsersCreatedInLastDays(7);
        logger.info("Found {} users created in last 7 days", recentUsers.size());
        
        // Complex queries with joins
        List<User> usersWithNotifications = userRepository.findUsersWithPendingNotifications();
        logger.info("Found {} users with pending notifications", usersWithNotifications.size());
        
        // Aggregation queries
        long totalActiveUsers = userRepository.countByStatus(User.Status.ACTIVE);
        logger.info("Total active users (from repository): {}", totalActiveUsers);
        
        // Custom finder methods
        List<User> adminAndModeratorUsers = userRepository.findByRoleIn(
            List.of(User.Role.ADMIN, User.Role.MODERATOR)
        );
        logger.info("Found {} users with elevated privileges", adminAndModeratorUsers.size());
    }
}