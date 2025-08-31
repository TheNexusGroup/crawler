use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use uuid::Uuid;
use std::collections::HashMap;

/// User role enumeration with hierarchical permissions
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum UserRole {
    User,
    Moderator,
    Admin,
    SuperAdmin,
}

impl UserRole {
    /// Get the hierarchical level of the role (higher number = more permissions)
    pub fn level(&self) -> u8 {
        match self {
            UserRole::User => 1,
            UserRole::Moderator => 2,
            UserRole::Admin => 3,
            UserRole::SuperAdmin => 4,
        }
    }

    /// Check if this role can manage another role
    pub fn can_manage(&self, other: &UserRole) -> bool {
        self.level() > other.level()
    }

    /// Get all permissions for this role
    pub fn permissions(&self) -> Vec<&'static str> {
        match self {
            UserRole::User => vec!["read"],
            UserRole::Moderator => vec!["read", "write", "moderate"],
            UserRole::Admin => vec!["read", "write", "moderate", "admin", "delete"],
            UserRole::SuperAdmin => vec!["read", "write", "moderate", "admin", "delete", "super_admin"],
        }
    }

    /// Check if role has specific permission
    pub fn has_permission(&self, permission: &str) -> bool {
        self.permissions().contains(&permission)
    }
}

/// User account status
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum UserStatus {
    Active,
    Inactive,
    Suspended,
    Deleted,
}

impl UserStatus {
    pub fn is_active(&self) -> bool {
        matches!(self, UserStatus::Active)
    }

    pub fn can_authenticate(&self) -> bool {
        matches!(self, UserStatus::Active | UserStatus::Inactive)
    }
}

/// Main User struct with complex relationships
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct User {
    pub id: Uuid,
    pub email: String,
    pub username: String,
    pub first_name: String,
    pub last_name: String,
    pub role: UserRole,
    pub status: UserStatus,
    pub email_verified: bool,
    pub last_login: Option<DateTime<Utc>>,
    pub login_count: i64,
    pub failed_login_attempts: i32,
    pub password_hash: String,
    pub metadata: HashMap<String, serde_json::Value>,
    pub preferences: UserPreferences,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub deleted_at: Option<DateTime<Utc>>,
}

/// User preferences and settings
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UserPreferences {
    pub theme: String,
    pub language: String,
    pub timezone: String,
    pub notifications_enabled: bool,
    pub email_notifications: bool,
    pub two_factor_enabled: bool,
    pub custom_settings: HashMap<String, serde_json::Value>,
}

impl Default for UserPreferences {
    fn default() -> Self {
        Self {
            theme: "dark".to_string(),
            language: "en".to_string(),
            timezone: "UTC".to_string(),
            notifications_enabled: true,
            email_notifications: true,
            two_factor_enabled: false,
            custom_settings: HashMap::new(),
        }
    }
}

impl User {
    /// Create a new user with default values
    pub fn new(
        email: String,
        username: String,
        first_name: String,
        last_name: String,
        password_hash: String,
    ) -> Self {
        let now = Utc::now();
        
        Self {
            id: Uuid::new_v4(),
            email,
            username,
            first_name,
            last_name,
            role: UserRole::User,
            status: UserStatus::Active,
            email_verified: false,
            last_login: None,
            login_count: 0,
            failed_login_attempts: 0,
            password_hash,
            metadata: HashMap::new(),
            preferences: UserPreferences::default(),
            created_at: now,
            updated_at: now,
            deleted_at: None,
        }
    }

    /// Get the user's full name
    pub fn full_name(&self) -> String {
        format!("{} {}", self.first_name, self.last_name).trim().to_string()
    }

    /// Check if user is an admin
    pub fn is_admin(&self) -> bool {
        matches!(self.role, UserRole::Admin | UserRole::SuperAdmin)
    }

    /// Check if user is active and can authenticate
    pub fn can_authenticate(&self) -> bool {
        self.status.can_authenticate() && self.deleted_at.is_none()
    }

    /// Check if user is currently locked out due to failed attempts
    pub fn is_locked_out(&self) -> bool {
        self.failed_login_attempts >= 5
    }

    /// Check if user has a specific permission
    pub fn has_permission(&self, permission: &str) -> bool {
        if !self.can_authenticate() {
            return false;
        }
        self.role.has_permission(permission)
    }

    /// Record a successful login
    pub fn record_login(&mut self) {
        self.last_login = Some(Utc::now());
        self.login_count += 1;
        self.failed_login_attempts = 0;
        self.updated_at = Utc::now();
    }

    /// Record a failed login attempt
    pub fn record_failed_login(&mut self) {
        self.failed_login_attempts += 1;
        self.updated_at = Utc::now();
    }

    /// Reset failed login attempts
    pub fn reset_failed_attempts(&mut self) {
        self.failed_login_attempts = 0;
        self.updated_at = Utc::now();
    }

    /// Soft delete the user
    pub fn soft_delete(&mut self) {
        self.status = UserStatus::Deleted;
        self.deleted_at = Some(Utc::now());
        self.updated_at = Utc::now();
    }

    /// Validate user data
    pub fn validate(&self) -> Vec<String> {
        let mut errors = Vec::new();

        if self.email.is_empty() || !self.email.contains('@') {
            errors.push("Invalid email address".to_string());
        }

        if self.username.len() < 3 {
            errors.push("Username must be at least 3 characters".to_string());
        }

        if self.first_name.is_empty() {
            errors.push("First name is required".to_string());
        }

        if self.password_hash.is_empty() {
            errors.push("Password hash is required".to_string());
        }

        errors
    }

    /// Update user's timestamp
    pub fn touch(&mut self) {
        self.updated_at = Utc::now();
    }
}

/// Request struct for creating a new user
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CreateUserRequest {
    pub email: String,
    pub username: String,
    pub first_name: String,
    pub last_name: String,
    pub role: UserRole,
}

impl CreateUserRequest {
    pub fn validate(&self) -> Vec<String> {
        let mut errors = Vec::new();

        if self.email.is_empty() || !self.email.contains('@') {
            errors.push("Invalid email address".to_string());
        }

        if self.username.len() < 3 {
            errors.push("Username must be at least 3 characters".to_string());
        }

        if self.first_name.is_empty() {
            errors.push("First name is required".to_string());
        }

        if self.last_name.is_empty() {
            errors.push("Last name is required".to_string());
        }

        errors
    }
}

/// Request struct for updating user data
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UpdateUserRequest {
    pub email: Option<String>,
    pub username: Option<String>,
    pub first_name: Option<String>,
    pub last_name: Option<String>,
    pub role: Option<UserRole>,
    pub status: Option<UserStatus>,
    pub preferences: Option<UserPreferences>,
    pub metadata: Option<HashMap<String, serde_json::Value>>,
}

/// Filtering options for user queries
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct UserFilters {
    pub role: Option<UserRole>,
    pub status: Option<UserStatus>,
    pub email_verified: Option<bool>,
    pub created_after: Option<DateTime<Utc>>,
    pub created_before: Option<DateTime<Utc>>,
    pub last_login_after: Option<DateTime<Utc>>,
    pub search_term: Option<String>,
    pub limit: Option<i64>,
    pub offset: Option<i64>,
}

impl UserFilters {
    pub fn new() -> Self {
        Default::default()
    }

    pub fn with_role(mut self, role: UserRole) -> Self {
        self.role = Some(role);
        self
    }

    pub fn with_status(mut self, status: UserStatus) -> Self {
        self.status = Some(status);
        self
    }

    pub fn with_search(mut self, term: String) -> Self {
        self.search_term = Some(term);
        self
    }

    pub fn with_limit(mut self, limit: i64) -> Self {
        self.limit = Some(limit);
        self
    }
}