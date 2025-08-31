// Main library file exposing all modules

pub mod config;
pub mod database;
pub mod middleware;
pub mod models;
pub mod repositories;
pub mod services;
pub mod utils;

// Re-export commonly used types
pub use config::AppConfig;
pub use models::{User, UserRole, CreateUserRequest};
pub use services::{UserService, NotificationService, CacheService};
pub use database::Database;
pub use utils::{Logger, Metrics};
pub use middleware::AuthMiddleware;