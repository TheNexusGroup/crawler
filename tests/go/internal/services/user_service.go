package services

import (
	"context"
	"fmt"
	"time"

	"github.com/crawler/test-go/internal/database"
	"github.com/crawler/test-go/internal/models"
	"github.com/crawler/test-go/pkg/logger"
)

// UserService interface defines user-related operations
type UserService interface {
	CreateUser(ctx context.Context, user *models.User) (*models.User, error)
	GetUserByID(ctx context.Context, id uint64) (*models.User, error)
	GetUserByEmail(ctx context.Context, email string) (*models.User, error)
	GetUsers(ctx context.Context, filters *models.UserFilters) ([]*models.User, error)
	UpdateUser(ctx context.Context, id uint64, updates *models.UserUpdates) (*models.User, error)
	DeleteUser(ctx context.Context, id uint64) error
	GetUserStats(ctx context.Context) (*models.UserStats, error)
	BulkUpdateUsers(ctx context.Context, updates []*models.BulkUserUpdate) error
	ValidateUser(user *models.User) error
}

// userService implements UserService interface
type userService struct {
	db       database.Database
	logger   logger.Logger
	cache    CacheService
	validator UserValidator
}

// UserValidator interface for user validation
type UserValidator interface {
	ValidateEmail(email string) error
	ValidatePassword(password string) error
	ValidateRole(role models.UserRole) error
	ValidateUserData(user *models.User) []string
}

// NewUserService creates a new user service instance
func NewUserService(db database.Database, logger logger.Logger, cache CacheService) UserService {
	return &userService{
		db:        db,
		logger:    logger,
		cache:     cache,
		validator: NewUserValidator(),
	}
}

// CreateUser creates a new user in the database
func (s *userService) CreateUser(ctx context.Context, user *models.User) (*models.User, error) {
	s.logger.Debug("Creating new user", "email", user.Email)

	// Validate user data
	if err := s.ValidateUser(user); err != nil {
		return nil, fmt.Errorf("user validation failed: %w", err)
	}

	// Check if user already exists
	existingUser, err := s.GetUserByEmail(ctx, user.Email)
	if err == nil && existingUser != nil {
		return nil, fmt.Errorf("user with email %s already exists", user.Email)
	}

	// Set timestamps
	now := time.Now()
	user.CreatedAt = now
	user.UpdatedAt = now

	// Create user in database
	if err := s.db.CreateUser(ctx, user); err != nil {
		s.logger.Error("Failed to create user", "error", err, "email", user.Email)
		return nil, fmt.Errorf("failed to create user: %w", err)
	}

	// Cache the user
	cacheKey := s.userCacheKey(user.ID)
	if err := s.cache.Set(ctx, cacheKey, user, 5*time.Minute); err != nil {
		s.logger.Warn("Failed to cache user", "error", err, "user_id", user.ID)
	}

	s.logger.Info("User created successfully", "user_id", user.ID, "email", user.Email)
	return user, nil
}

// GetUserByID retrieves a user by ID with caching
func (s *userService) GetUserByID(ctx context.Context, id uint64) (*models.User, error) {
	cacheKey := s.userCacheKey(id)

	// Try to get from cache first
	var user models.User
	if err := s.cache.Get(ctx, cacheKey, &user); err == nil {
		s.logger.Debug("User found in cache", "user_id", id)
		return &user, nil
	}

	// Get from database
	dbUser, err := s.db.GetUserByID(ctx, id)
	if err != nil {
		return nil, fmt.Errorf("failed to get user by ID: %w", err)
	}

	if dbUser == nil {
		return nil, nil
	}

	// Cache the result
	if err := s.cache.Set(ctx, cacheKey, dbUser, 5*time.Minute); err != nil {
		s.logger.Warn("Failed to cache user", "error", err, "user_id", id)
	}

	return dbUser, nil
}

// GetUserByEmail retrieves a user by email
func (s *userService) GetUserByEmail(ctx context.Context, email string) (*models.User, error) {
	cacheKey := fmt.Sprintf("user:email:%s", email)

	// Try cache first
	var user models.User
	if err := s.cache.Get(ctx, cacheKey, &user); err == nil {
		return &user, nil
	}

	// Get from database
	dbUser, err := s.db.GetUserByEmail(ctx, email)
	if err != nil {
		return nil, fmt.Errorf("failed to get user by email: %w", err)
	}

	if dbUser != nil {
		// Cache the result
		if err := s.cache.Set(ctx, cacheKey, dbUser, 5*time.Minute); err != nil {
			s.logger.Warn("Failed to cache user by email", "error", err, "email", email)
		}
	}

	return dbUser, nil
}

// GetUsers retrieves users with optional filtering
func (s *userService) GetUsers(ctx context.Context, filters *models.UserFilters) ([]*models.User, error) {
	if filters == nil {
		filters = &models.UserFilters{}
	}

	users, err := s.db.GetUsers(ctx, filters)
	if err != nil {
		return nil, fmt.Errorf("failed to get users: %w", err)
	}

	return users, nil
}

// UpdateUser updates an existing user
func (s *userService) UpdateUser(ctx context.Context, id uint64, updates *models.UserUpdates) (*models.User, error) {
	s.logger.Debug("Updating user", "user_id", id)

	// Get existing user
	existingUser, err := s.GetUserByID(ctx, id)
	if err != nil {
		return nil, err
	}
	if existingUser == nil {
		return nil, fmt.Errorf("user with ID %d not found", id)
	}

	// Apply updates
	updatedUser := s.applyUserUpdates(existingUser, updates)

	// Validate updated user
	if err := s.ValidateUser(updatedUser); err != nil {
		return nil, fmt.Errorf("user validation failed: %w", err)
	}

	// Update in database
	updatedUser.UpdatedAt = time.Now()
	if err := s.db.UpdateUser(ctx, updatedUser); err != nil {
		return nil, fmt.Errorf("failed to update user: %w", err)
	}

	// Invalidate cache entries
	s.invalidateUserCache(ctx, id, existingUser.Email, updatedUser.Email)

	s.logger.Info("User updated successfully", "user_id", id)
	return updatedUser, nil
}

// DeleteUser soft deletes a user
func (s *userService) DeleteUser(ctx context.Context, id uint64) error {
	s.logger.Debug("Deleting user", "user_id", id)

	user, err := s.GetUserByID(ctx, id)
	if err != nil {
		return err
	}
	if user == nil {
		return fmt.Errorf("user with ID %d not found", id)
	}

	// Soft delete in database
	if err := s.db.DeleteUser(ctx, id); err != nil {
		return fmt.Errorf("failed to delete user: %w", err)
	}

	// Invalidate cache
	s.invalidateUserCache(ctx, id, user.Email, "")

	s.logger.Info("User deleted successfully", "user_id", id)
	return nil
}

// GetUserStats returns user statistics
func (s *userService) GetUserStats(ctx context.Context) (*models.UserStats, error) {
	stats, err := s.db.GetUserStats(ctx)
	if err != nil {
		return nil, fmt.Errorf("failed to get user stats: %w", err)
	}

	return stats, nil
}

// BulkUpdateUsers performs bulk updates on multiple users
func (s *userService) BulkUpdateUsers(ctx context.Context, updates []*models.BulkUserUpdate) error {
	s.logger.Debug("Performing bulk user updates", "count", len(updates))

	if err := s.db.BulkUpdateUsers(ctx, updates); err != nil {
		return fmt.Errorf("bulk update failed: %w", err)
	}

	// Invalidate cache for updated users
	for _, update := range updates {
		s.invalidateUserCache(ctx, update.UserID, "", "")
	}

	s.logger.Info("Bulk user updates completed", "count", len(updates))
	return nil
}

// ValidateUser validates user data using the validator
func (s *userService) ValidateUser(user *models.User) error {
	errors := s.validator.ValidateUserData(user)
	if len(errors) > 0 {
		return fmt.Errorf("validation errors: %v", errors)
	}
	return nil
}

// Helper methods

func (s *userService) userCacheKey(id uint64) string {
	return fmt.Sprintf("user:id:%d", id)
}

func (s *userService) applyUserUpdates(user *models.User, updates *models.UserUpdates) *models.User {
	updatedUser := *user // Copy the user

	if updates.Email != nil {
		updatedUser.Email = *updates.Email
	}
	if updates.FirstName != nil {
		updatedUser.FirstName = *updates.FirstName
	}
	if updates.LastName != nil {
		updatedUser.LastName = *updates.LastName
	}
	if updates.Role != nil {
		updatedUser.Role = *updates.Role
	}
	if updates.Status != nil {
		updatedUser.Status = *updates.Status
	}

	return &updatedUser
}

func (s *userService) invalidateUserCache(ctx context.Context, id uint64, oldEmail, newEmail string) {
	// Invalidate ID cache
	cacheKey := s.userCacheKey(id)
	if err := s.cache.Delete(ctx, cacheKey); err != nil {
		s.logger.Warn("Failed to invalidate user ID cache", "error", err, "user_id", id)
	}

	// Invalidate old email cache
	if oldEmail != "" {
		emailKey := fmt.Sprintf("user:email:%s", oldEmail)
		if err := s.cache.Delete(ctx, emailKey); err != nil {
			s.logger.Warn("Failed to invalidate old email cache", "error", err, "email", oldEmail)
		}
	}

	// Invalidate new email cache if different
	if newEmail != "" && newEmail != oldEmail {
		emailKey := fmt.Sprintf("user:email:%s", newEmail)
		if err := s.cache.Delete(ctx, emailKey); err != nil {
			s.logger.Warn("Failed to invalidate new email cache", "error", err, "email", newEmail)
		}
	}
}