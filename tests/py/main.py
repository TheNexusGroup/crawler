#!/usr/bin/env python3
"""
Main entry point for the Python test application.
Demonstrates complex dependency relationships across modules.
"""

import sys
import asyncio
from typing import Optional, List
from dataclasses import dataclass

from services.user_service import UserService
from services.notification_service import NotificationService
from models.user import User, UserRole
from database.connection import DatabaseConnection
from utils.logger import Logger
from utils.config import Config
from decorators.timing import timing_decorator
from decorators.cache import cached


@dataclass
class AppConfig:
    database_url: str
    debug: bool = False
    log_level: str = "INFO"
    cache_ttl: int = 300


class Application:
    def __init__(self, config: AppConfig):
        self.config = config
        self.logger = Logger(__name__)
        self.db = DatabaseConnection(config.database_url)
        self.user_service = UserService(self.db)
        self.notification_service = NotificationService()
        
    @timing_decorator
    async def initialize(self):
        """Initialize the application components."""
        try:
            await self.db.connect()
            await self.user_service.initialize()
            await self.notification_service.initialize()
            self.logger.info("Application initialized successfully")
        except Exception as e:
            self.logger.error(f"Failed to initialize application: {e}")
            raise
    
    @cached(ttl=300)
    async def get_active_users(self) -> List[User]:
        """Get all active users with caching."""
        return await self.user_service.get_users_by_status("active")
    
    async def process_user_batch(self, user_ids: List[int]) -> None:
        """Process a batch of users with notifications."""
        users = []
        
        for user_id in user_ids:
            try:
                user = await self.user_service.get_user_by_id(user_id)
                if user and user.is_active():
                    users.append(user)
            except Exception as e:
                self.logger.warning(f"Failed to process user {user_id}: {e}")
                continue
        
        if users:
            await self.notification_service.send_bulk_notifications(
                users, 
                "batch_processing_complete",
                {"batch_size": len(users)}
            )
    
    async def run(self):
        """Main application loop."""
        await self.initialize()
        
        try:
            # Example workflow
            active_users = await self.get_active_users()
            self.logger.info(f"Found {len(active_users)} active users")
            
            admin_users = [u for u in active_users if u.role == UserRole.ADMIN]
            if admin_users:
                await self.process_user_batch([u.id for u in admin_users[:10]])
                
        except Exception as e:
            self.logger.error(f"Application error: {e}")
            raise
        finally:
            await self.cleanup()
    
    async def cleanup(self):
        """Cleanup application resources."""
        if self.notification_service:
            await self.notification_service.close()
        if self.db:
            await self.db.close()
        self.logger.info("Application cleanup completed")


async def main():
    """Main function."""
    config = Config.load_from_env()
    app_config = AppConfig(
        database_url=config.get("DATABASE_URL", "sqlite:///test.db"),
        debug=config.get("DEBUG", False),
        log_level=config.get("LOG_LEVEL", "INFO")
    )
    
    app = Application(app_config)
    
    try:
        await app.run()
    except KeyboardInterrupt:
        print("Application interrupted by user")
    except Exception as e:
        print(f"Application failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    asyncio.run(main())