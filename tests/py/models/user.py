"""
User model with complex relationships and validation.
"""

from enum import Enum
from typing import Optional, Dict, Any, List
from datetime import datetime
from dataclasses import dataclass, field

from utils.validation import ValidationMixin
from decorators.cache import cached


class UserRole(Enum):
    """User role enumeration."""
    USER = "user"
    MODERATOR = "moderator"
    ADMIN = "admin"
    SUPER_ADMIN = "super_admin"


class UserStatus(Enum):
    """User status enumeration."""
    ACTIVE = "active"
    INACTIVE = "inactive"
    SUSPENDED = "suspended"
    DELETED = "deleted"


@dataclass
class UserPermissions:
    """User permissions data structure."""
    can_read: bool = True
    can_write: bool = False
    can_delete: bool = False
    can_admin: bool = False
    custom_permissions: Dict[str, bool] = field(default_factory=dict)


class User(ValidationMixin):
    """
    User model with complex validation and relationships.
    """
    
    def __init__(
        self,
        user_id: Optional[int] = None,
        email: str = "",
        username: str = "",
        first_name: str = "",
        last_name: str = "",
        role: UserRole = UserRole.USER,
        status: UserStatus = UserStatus.ACTIVE,
        permissions: Optional[UserPermissions] = None,
        metadata: Optional[Dict[str, Any]] = None
    ):
        self.id = user_id
        self.email = email
        self.username = username
        self.first_name = first_name
        self.last_name = last_name
        self.role = role
        self.status = status
        self.permissions = permissions or UserPermissions()
        self.metadata = metadata or {}
        self.created_at = datetime.utcnow()
        self.updated_at = datetime.utcnow()
        self._relationships: Dict[str, List['User']] = {}
    
    @property
    def full_name(self) -> str:
        """Get user's full name."""
        return f"{self.first_name} {self.last_name}".strip()
    
    @property
    def is_admin(self) -> bool:
        """Check if user has admin role."""
        return self.role in [UserRole.ADMIN, UserRole.SUPER_ADMIN]
    
    def is_active(self) -> bool:
        """Check if user is active."""
        return self.status == UserStatus.ACTIVE
    
    def has_permission(self, permission: str) -> bool:
        """Check if user has specific permission."""
        if self.is_admin:
            return True
            
        # Check standard permissions
        standard_perms = {
            'read': self.permissions.can_read,
            'write': self.permissions.can_write,
            'delete': self.permissions.can_delete,
            'admin': self.permissions.can_admin
        }
        
        if permission in standard_perms:
            return standard_perms[permission]
        
        # Check custom permissions
        return self.permissions.custom_permissions.get(permission, False)
    
    def add_relationship(self, relationship_type: str, other_user: 'User') -> None:
        """Add relationship to another user."""
        if relationship_type not in self._relationships:
            self._relationships[relationship_type] = []
        
        if other_user not in self._relationships[relationship_type]:
            self._relationships[relationship_type].append(other_user)
    
    def get_relationships(self, relationship_type: str) -> List['User']:
        """Get users with specific relationship."""
        return self._relationships.get(relationship_type, [])
    
    @cached(ttl=60)
    def get_role_hierarchy_level(self) -> int:
        """Get numeric role hierarchy level for comparisons."""
        hierarchy = {
            UserRole.USER: 1,
            UserRole.MODERATOR: 2,
            UserRole.ADMIN: 3,
            UserRole.SUPER_ADMIN: 4
        }
        return hierarchy.get(self.role, 0)
    
    def can_manage_user(self, other_user: 'User') -> bool:
        """Check if this user can manage another user."""
        return (
            self.is_admin and 
            self.get_role_hierarchy_level() > other_user.get_role_hierarchy_level()
        )
    
    def validate(self) -> Dict[str, List[str]]:
        """Validate user data."""
        errors = {}
        
        if not self.email or '@' not in self.email:
            errors['email'] = ['Valid email address is required']
        
        if not self.username or len(self.username) < 3:
            errors['username'] = ['Username must be at least 3 characters']
        
        if not self.first_name:
            errors['first_name'] = ['First name is required']
        
        if not isinstance(self.role, UserRole):
            errors['role'] = ['Invalid user role']
        
        if not isinstance(self.status, UserStatus):
            errors['status'] = ['Invalid user status']
        
        return errors
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert user to dictionary."""
        return {
            'id': self.id,
            'email': self.email,
            'username': self.username,
            'first_name': self.first_name,
            'last_name': self.last_name,
            'full_name': self.full_name,
            'role': self.role.value,
            'status': self.status.value,
            'permissions': {
                'can_read': self.permissions.can_read,
                'can_write': self.permissions.can_write,
                'can_delete': self.permissions.can_delete,
                'can_admin': self.permissions.can_admin,
                'custom_permissions': self.permissions.custom_permissions
            },
            'metadata': self.metadata,
            'created_at': self.created_at.isoformat(),
            'updated_at': self.updated_at.isoformat(),
            'is_admin': self.is_admin,
            'is_active': self.is_active()
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'User':
        """Create user from dictionary."""
        permissions = UserPermissions(
            can_read=data.get('permissions', {}).get('can_read', True),
            can_write=data.get('permissions', {}).get('can_write', False),
            can_delete=data.get('permissions', {}).get('can_delete', False),
            can_admin=data.get('permissions', {}).get('can_admin', False),
            custom_permissions=data.get('permissions', {}).get('custom_permissions', {})
        )
        
        return cls(
            user_id=data.get('id'),
            email=data.get('email', ''),
            username=data.get('username', ''),
            first_name=data.get('first_name', ''),
            last_name=data.get('last_name', ''),
            role=UserRole(data.get('role', UserRole.USER.value)),
            status=UserStatus(data.get('status', UserStatus.ACTIVE.value)),
            permissions=permissions,
            metadata=data.get('metadata', {})
        )
    
    def __repr__(self) -> str:
        return f"User(id={self.id}, username='{self.username}', role={self.role.value})"
    
    def __eq__(self, other: object) -> bool:
        if not isinstance(other, User):
            return False
        return self.id == other.id and self.id is not None
    
    def __hash__(self) -> int:
        return hash(self.id) if self.id else hash(self.username)