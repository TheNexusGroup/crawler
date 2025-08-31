# frozen_string_literal: true

require_relative 'services/user_service'
require_relative 'services/notification_service'
require_relative 'services/cache_service'
require_relative 'repositories/user_repository'
require_relative 'models/user'
require_relative '../lib/utils/logger'
require_relative '../lib/modules/authentication_module'
require_relative '../lib/concerns/cacheable'

##
# Main application class demonstrating complex Ruby dependency relationships.
# 
# This application showcases various Ruby patterns including:
# - Module mixins and concerns
# - Service layer architecture
# - Repository pattern
# - Dependency injection
# - Metaprogramming features
# - Block and proc usage
# - Ruby-specific patterns like decorators and validators
class Application
  include AuthenticationModule
  include Cacheable

  attr_reader :user_service, :notification_service, :cache_service, :logger
  
  def initialize(dependencies = {})
    @user_service = dependencies[:user_service] || Services::UserService.new
    @notification_service = dependencies[:notification_service] || Services::NotificationService.new
    @cache_service = dependencies[:cache_service] || Services::CacheService.new
    @logger = dependencies[:logger] || Utils::Logger.new('Application')
    @initialized = false
  end

  ##
  # Initialize the application and all its dependencies.
  def initialize_application
    logger.info('Initializing Crawler Test Ruby Application')
    
    begin
      # Initialize database connections
      setup_database_connections
      
      # Initialize cache service
      cache_service.connect
      logger.info('Cache service connected')
      
      # Initialize services with dependency injection
      user_service.initialize_with_dependencies(
        cache_service: cache_service,
        logger: logger
      )
      
      notification_service.initialize_with_dependencies(
        user_service: user_service,
        cache_service: cache_service,
        logger: logger
      )
      
      @initialized = true
      logger.info('All services initialized successfully')
      
    rescue StandardError => e
      logger.error("Failed to initialize application: #{e.message}")
      logger.error(e.backtrace.join("\n"))
      raise e
    end
  end

  ##
  # Run the main application workflow.
  def run
    raise 'Application not initialized' unless @initialized
    
    logger.info('Starting main application workflow')
    
    begin
      execute_sample_workflow
      logger.info('Main workflow completed successfully')
    rescue StandardError => e
      logger.error("Main workflow failed: #{e.message}")
      raise e
    end
  end

  ##
  # Create a sample user using block syntax and hash parameters.
  def create_sample_user(**params)
    user_data = {
      email: params[:email],
      username: params[:username],
      first_name: params[:first_name],
      last_name: params[:last_name],
      role: params[:role] || :user,
      status: :active,
      email_verified: true
    }
    
    # Use block for validation
    user_service.create_user(user_data) do |user|
      logger.info("Created user: #{user.username} (#{user.id})")
      
      # Send welcome email using proc
      send_welcome_email = proc do |user_instance|
        notification_service.send_welcome_notification(user_instance)
      end
      
      send_welcome_email.call(user)
    end
  end

  private

  ##
  # Execute a sample workflow demonstrating Ruby patterns and service dependencies.
  def execute_sample_workflow
    logger.info('Starting sample workflow execution')
    
    # Create sample users using Ruby's flexible syntax
    users = create_sample_users
    
    # Demonstrate service interactions
    demonstrate_user_operations(users)
    demonstrate_service_interactions(users)
    demonstrate_ruby_patterns(users)
    demonstrate_caching_with_concerns
    
    logger.info('Sample workflow completed')
  end

  ##
  # Create sample users demonstrating Ruby's expressive syntax.
  def create_sample_users
    users = {}
    
    # Use symbols and hash shortcuts
    user_definitions = [
      { email: 'admin@example.com', username: 'admin_user', 
        first_name: 'Admin', last_name: 'User', role: :admin },
      { email: 'moderator@example.com', username: 'mod_user', 
        first_name: 'Moderator', last_name: 'User', role: :moderator },
      { email: 'user@example.com', username: 'regular_user', 
        first_name: 'Regular', last_name: 'User', role: :user }
    ]
    
    # Use each_with_object for functional-style processing
    user_definitions.each_with_object(users) do |definition, hash|
      user = create_sample_user(**definition)
      hash[definition[:role]] = user
    end
  end

  ##
  # Demonstrate user service operations with Ruby idioms.
  def demonstrate_user_operations(users)
    logger.info('Demonstrating user service operations')
    
    # Use safe navigation operator
    active_users = user_service&.get_active_users || []
    logger.info("Found #{active_users.count} active users")
    
    # Demonstrate filtering with blocks
    admin_users = active_users.select(&:admin?)
    moderator_users = active_users.filter { |user| user.role == :moderator }
    
    logger.info("Admin users: #{admin_users.count}, Moderator users: #{moderator_users.count}")
    
    # Use range and case statements
    user_count = active_users.count
    status = case user_count
             when 0
               'No users found'
             when 1..5
               'Few users'
             when 6..20
               'Moderate user base'
             else
               'Large user base'
             end
    
    logger.info("User base status: #{status}")
  end

  ##
  # Demonstrate service interactions using Ruby patterns.
  def demonstrate_service_interactions(users)
    logger.info('Demonstrating service interactions')
    
    # Use parallel assignment
    admin_user, moderator_user, regular_user = users[:admin], users[:moderator], users[:user]
    
    # Send notifications using method chaining
    [regular_user, moderator_user].each do |user|
      notification_service
        .create_notification(user)
        .with_type(:welcome)
        .with_priority(:normal)
        .deliver_later
    end
    
    # Send special admin notification with block
    notification_service.send_notification(admin_user) do |notification|
      notification.type = :admin_welcome
      notification.priority = :high
      notification.template = 'admin_welcome_template'
    end
    
    # Demonstrate user promotion with callbacks
    user_service.promote_user(regular_user.id, to: :moderator) do |promoted_user|
      logger.info("User #{promoted_user.username} promoted to #{promoted_user.role}")
      
      # Send promotion notification
      notification_service.send_promotion_notification(promoted_user)
    end
  end

  ##
  # Demonstrate Ruby-specific patterns and metaprogramming.
  def demonstrate_ruby_patterns(users)
    logger.info('Demonstrating Ruby patterns and metaprogramming')
    
    # Use method_missing delegation pattern
    user_stats = user_service.statistics
    
    # Dynamic method calls
    %w[total_users active_users inactive_users].each do |stat_method|
      value = user_stats.send(stat_method)
      logger.info("#{stat_method}: #{value}")
    end
    
    # Use tap for side effects
    users[:admin].tap do |admin|
      admin.last_login = Time.current
      admin.increment_login_count
      user_service.update_user(admin)
    end
    
    # Demonstrate string interpolation and heredoc
    report = <<~REPORT
      User Report
      ===========
      Total Users: #{users.count}
      Admin Users: #{users.select { |_, user| user.admin? }.count}
      Active Users: #{user_service.get_active_users.count}
      
      Generated at: #{Time.current.strftime('%Y-%m-%d %H:%M:%S')}
    REPORT
    
    logger.info(report)
  end

  ##
  # Demonstrate caching using concerns.
  def demonstrate_caching_with_concerns
    logger.info('Demonstrating caching with concerns')
    
    # Use the Cacheable concern
    cache_key = 'user_statistics'
    
    stats = cached(cache_key, expires_in: 1.hour) do
      logger.info('Calculating fresh user statistics')
      user_service.calculate_detailed_statistics
    end
    
    logger.info("Cached statistics: #{stats}")
    
    # Cache invalidation
    invalidate_cache(cache_key)
    logger.info("Cache invalidated for key: #{cache_key}")
  end

  ##
  # Setup database connections with error handling.
  def setup_database_connections
    # Simulate database connection setup
    logger.info('Setting up database connections')
    
    # Ruby's exception handling with ensure
    begin
      # Database connection logic would go here
      sleep(0.1) # Simulate connection time
      logger.info('Database connections established')
    rescue StandardError => e
      logger.error("Database connection failed: #{e.message}")
      raise e
    ensure
      logger.debug('Database connection attempt completed')
    end
  end

  ##
  # Graceful shutdown of the application.
  def shutdown
    logger.info('Starting application shutdown')
    
    begin
      # Shutdown services in reverse dependency order using safe navigation
      notification_service&.shutdown
      user_service&.shutdown
      cache_service&.disconnect
      
      logger.info('Application shutdown completed successfully')
    rescue StandardError => e
      logger.error("Error during application shutdown: #{e.message}")
    end
  end
end