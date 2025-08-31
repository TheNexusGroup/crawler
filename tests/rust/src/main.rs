use anyhow::Result;
use std::sync::Arc;
use tokio::signal;
use tracing::{info, error};

use crawler_test_rust::{
    config::AppConfig,
    services::{UserService, NotificationService, CacheService},
    database::Database,
    models::{User, UserRole, CreateUserRequest},
    utils::{Logger, Metrics},
    middleware::AuthMiddleware,
    repositories::{UserRepository, PostgresUserRepository},
};

/// Application state containing all services and dependencies
#[derive(Clone)]
pub struct AppState {
    pub user_service: Arc<dyn UserService>,
    pub notification_service: Arc<dyn NotificationService>,
    pub cache_service: Arc<dyn CacheService>,
    pub database: Arc<dyn Database>,
    pub logger: Arc<Logger>,
    pub metrics: Arc<Metrics>,
}

/// Main application struct
pub struct Application {
    state: AppState,
    config: AppConfig,
}

impl Application {
    /// Create a new application instance
    pub async fn new() -> Result<Self> {
        let config = AppConfig::from_env()?;
        let logger = Arc::new(Logger::new(&config.log_level)?);
        
        info!("Initializing application with config: {:?}", config);

        // Initialize database connection
        let database = Arc::new(
            Database::connect(&config.database_url).await?
        );

        // Run database migrations
        database.migrate().await?;

        // Initialize cache service
        let cache_service = Arc::new(
            CacheService::new(&config.redis_url).await?
        );

        // Initialize metrics
        let metrics = Arc::new(Metrics::new()?);

        // Initialize repository layer
        let user_repo: Arc<dyn UserRepository> = Arc::new(
            PostgresUserRepository::new(database.clone())
        );

        // Initialize services
        let user_service = Arc::new(
            UserService::new(user_repo, cache_service.clone(), logger.clone()).await?
        );

        let notification_service = Arc::new(
            NotificationService::new(&config.notification_config, logger.clone()).await?
        );

        let state = AppState {
            user_service,
            notification_service,
            cache_service,
            database,
            logger,
            metrics,
        };

        Ok(Self { state, config })
    }

    /// Initialize the application and all its components
    pub async fn initialize(&self) -> Result<()> {
        info!("Starting application initialization");

        // Initialize services
        self.state.user_service.initialize().await?;
        self.state.notification_service.initialize().await?;
        self.state.cache_service.health_check().await?;

        // Verify database connectivity
        self.state.database.ping().await?;

        info!("Application initialization completed successfully");
        Ok(())
    }

    /// Run the main application logic
    pub async fn run(&self) -> Result<()> {
        self.initialize().await?;

        info!("Starting main application workflow");

        // Example workflow demonstrating complex dependency relationships
        let workflow_result = self.execute_sample_workflow().await;

        match workflow_result {
            Ok(_) => {
                info!("Sample workflow completed successfully");
            }
            Err(e) => {
                error!("Sample workflow failed: {}", e);
                return Err(e);
            }
        }

        // Wait for shutdown signal
        self.wait_for_shutdown().await;

        Ok(())
    }

    /// Execute a sample workflow showing inter-service dependencies
    async fn execute_sample_workflow(&self) -> Result<()> {
        let logger = &self.state.logger;
        let user_service = &self.state.user_service;
        let notification_service = &self.state.notification_service;

        // Create sample users with different roles
        let admin_request = CreateUserRequest {
            email: "admin@example.com".to_string(),
            username: "admin_user".to_string(),
            first_name: "Admin".to_string(),
            last_name: "User".to_string(),
            role: UserRole::Admin,
        };

        let regular_request = CreateUserRequest {
            email: "user@example.com".to_string(),
            username: "regular_user".to_string(),
            first_name: "Regular".to_string(),
            last_name: "User".to_string(),
            role: UserRole::User,
        };

        // Create users
        let admin_user = user_service.create_user(admin_request).await?;
        let regular_user = user_service.create_user(regular_request).await?;

        logger.info(&format!("Created admin user: {}", admin_user.id));
        logger.info(&format!("Created regular user: {}", regular_user.id));

        // Demonstrate cross-service dependencies
        let users = user_service.get_active_users().await?;
        logger.info(&format!("Found {} active users", users.len()));

        // Send notifications using the notification service
        for user in &users {
            if user.role == UserRole::Admin {
                notification_service
                    .send_welcome_notification(user.id, &user.email)
                    .await?;
            }
        }

        // Demonstrate caching
        let cached_user = user_service.get_user_by_id(admin_user.id).await?;
        match cached_user {
            Some(user) => {
                logger.info(&format!("Retrieved cached user: {}", user.username));
            }
            None => {
                logger.warn(&format!("User {} not found", admin_user.id));
            }
        }

        // Update metrics
        self.state.metrics.increment_counter("workflow.completed").await?;
        self.state.metrics.record_duration("workflow.duration", std::time::Duration::from_millis(500)).await?;

        Ok(())
    }

    /// Wait for shutdown signal
    async fn wait_for_shutdown(&self) {
        let ctrl_c = signal::ctrl_c();
        
        tokio::select! {
            _ = ctrl_c => {
                info!("Received Ctrl+C, shutting down...");
            }
        }

        self.shutdown().await;
    }

    /// Graceful shutdown
    async fn shutdown(&self) {
        info!("Starting graceful shutdown");

        // Shutdown services in reverse dependency order
        if let Err(e) = self.state.notification_service.shutdown().await {
            error!("Error shutting down notification service: {}", e);
        }

        if let Err(e) = self.state.user_service.shutdown().await {
            error!("Error shutting down user service: {}", e);
        }

        if let Err(e) = self.state.cache_service.close().await {
            error!("Error closing cache service: {}", e);
        }

        if let Err(e) = self.state.database.close().await {
            error!("Error closing database connection: {}", e);
        }

        info!("Graceful shutdown completed");
    }
}

#[tokio::main]
async fn main() -> Result<()> {
    // Initialize tracing
    tracing_subscriber::fmt()
        .with_env_filter("info,crawler_test_rust=debug")
        .init();

    info!("Starting Crawler Test Rust Application");

    let app = Application::new().await?;
    
    if let Err(e) = app.run().await {
        error!("Application failed: {}", e);
        std::process::exit(1);
    }

    info!("Application completed successfully");
    Ok(())
}