package main

import (
	"context"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/crawler/test-go/internal/api"
	"github.com/crawler/test-go/internal/config"
	"github.com/crawler/test-go/internal/database"
	"github.com/crawler/test-go/internal/services"
	"github.com/crawler/test-go/pkg/logger"
	"github.com/crawler/test-go/pkg/middleware"
	"github.com/gin-gonic/gin"
)

// Application represents the main application structure
type Application struct {
	config     *config.Config
	logger     logger.Logger
	db         database.Database
	userSvc    services.UserService
	authSvc    services.AuthService
	cacheSvc   services.CacheService
	server     *http.Server
	router     *gin.Engine
}

// NewApplication creates a new application instance
func NewApplication() *Application {
	return &Application{}
}

// Initialize sets up all application components
func (app *Application) Initialize() error {
	var err error

	// Load configuration
	app.config, err = config.Load()
	if err != nil {
		return fmt.Errorf("failed to load config: %w", err)
	}

	// Initialize logger
	app.logger = logger.New(app.config.LogLevel)
	app.logger.Info("Starting application initialization")

	// Initialize database
	app.db, err = database.New(app.config.DatabaseURL)
	if err != nil {
		return fmt.Errorf("failed to initialize database: %w", err)
	}

	// Initialize cache service
	app.cacheSvc = services.NewCacheService(app.config.RedisURL, app.logger)
	if err := app.cacheSvc.Connect(); err != nil {
		return fmt.Errorf("failed to connect to cache: %w", err)
	}

	// Initialize services
	app.userSvc = services.NewUserService(app.db, app.logger, app.cacheSvc)
	app.authSvc = services.NewAuthService(app.userSvc, app.config.JWTSecret, app.logger)

	// Setup router and middleware
	app.setupRouter()

	// Create HTTP server
	app.server = &http.Server{
		Addr:         fmt.Sprintf(":%d", app.config.Port),
		Handler:      app.router,
		ReadTimeout:  15 * time.Second,
		WriteTimeout: 15 * time.Second,
		IdleTimeout:  60 * time.Second,
	}

	app.logger.Info("Application initialized successfully")
	return nil
}

// setupRouter configures the HTTP router and middleware
func (app *Application) setupRouter() {
	if app.config.Environment == "production" {
		gin.SetMode(gin.ReleaseMode)
	}

	app.router = gin.New()

	// Global middleware
	app.router.Use(gin.Logger())
	app.router.Use(gin.Recovery())
	app.router.Use(middleware.CORS())
	app.router.Use(middleware.RequestID())
	app.router.Use(middleware.RateLimit(100)) // 100 requests per minute

	// Health check endpoints
	app.router.GET("/health", app.healthCheck)
	app.router.GET("/ready", app.readinessCheck)

	// API routes
	apiGroup := app.router.Group("/api/v1")
	{
		// Authentication routes
		auth := apiGroup.Group("/auth")
		authHandler := api.NewAuthHandler(app.authSvc, app.logger)
		auth.POST("/login", authHandler.Login)
		auth.POST("/register", authHandler.Register)
		auth.POST("/refresh", authHandler.RefreshToken)

		// Protected user routes
		users := apiGroup.Group("/users")
		users.Use(middleware.AuthMiddleware(app.authSvc))
		userHandler := api.NewUserHandler(app.userSvc, app.logger)
		
		users.GET("", userHandler.GetUsers)
		users.GET("/:id", userHandler.GetUser)
		users.PUT("/:id", userHandler.UpdateUser)
		users.DELETE("/:id", userHandler.DeleteUser)

		// Admin only routes
		admin := users.Group("")
		admin.Use(middleware.RequireRole("admin"))
		admin.GET("/admin/stats", userHandler.GetUserStats)
		admin.POST("/admin/bulk-update", userHandler.BulkUpdateUsers)
	}
}

// healthCheck returns basic health status
func (app *Application) healthCheck(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{
		"status":    "healthy",
		"timestamp": time.Now().UTC().Format(time.RFC3339),
		"version":   app.config.Version,
	})
}

// readinessCheck checks if all dependencies are ready
func (app *Application) readinessCheck(c *gin.Context) {
	checks := make(map[string]bool)

	// Check database
	checks["database"] = app.db.Ping() == nil

	// Check cache
	checks["cache"] = app.cacheSvc.Ping() == nil

	allReady := true
	for _, ready := range checks {
		if !ready {
			allReady = false
			break
		}
	}

	status := http.StatusOK
	if !allReady {
		status = http.StatusServiceUnavailable
	}

	c.JSON(status, gin.H{
		"ready":     allReady,
		"checks":    checks,
		"timestamp": time.Now().UTC().Format(time.RFC3339),
	})
}

// Start runs the HTTP server
func (app *Application) Start() error {
	app.logger.Info(fmt.Sprintf("Starting server on port %d", app.config.Port))
	
	if err := app.server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		return fmt.Errorf("server startup failed: %w", err)
	}
	
	return nil
}

// Shutdown gracefully shuts down the application
func (app *Application) Shutdown(ctx context.Context) error {
	app.logger.Info("Shutting down application...")

	// Shutdown HTTP server
	if err := app.server.Shutdown(ctx); err != nil {
		app.logger.Error("Server shutdown failed", "error", err)
		return err
	}

	// Close database connection
	if app.db != nil {
		if err := app.db.Close(); err != nil {
			app.logger.Error("Database close failed", "error", err)
		}
	}

	// Close cache connection
	if app.cacheSvc != nil {
		if err := app.cacheSvc.Close(); err != nil {
			app.logger.Error("Cache close failed", "error", err)
		}
	}

	app.logger.Info("Application shutdown completed")
	return nil
}

func main() {
	app := NewApplication()

	// Initialize application
	if err := app.Initialize(); err != nil {
		log.Fatalf("Application initialization failed: %v", err)
	}

	// Setup graceful shutdown
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Handle shutdown signals
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	// Start server in goroutine
	go func() {
		if err := app.Start(); err != nil {
			app.logger.Error("Server failed", "error", err)
			cancel()
		}
	}()

	// Wait for shutdown signal
	select {
	case <-sigChan:
		app.logger.Info("Shutdown signal received")
	case <-ctx.Done():
		app.logger.Info("Context cancelled")
	}

	// Graceful shutdown with timeout
	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer shutdownCancel()

	if err := app.Shutdown(shutdownCtx); err != nil {
		log.Printf("Graceful shutdown failed: %v", err)
		os.Exit(1)
	}

	app.logger.Info("Application exited successfully")
}