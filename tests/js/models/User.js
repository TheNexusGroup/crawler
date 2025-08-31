export class User {
    constructor(userData) {
        this.id = userData.id || null;
        this.name = userData.name || '';
        this.email = userData.email || '';
        this.role = userData.role || 'user';
        this.createdAt = userData.createdAt || new Date();
        this.updatedAt = userData.updatedAt || new Date();
    }

    isAdmin() {
        return this.role === 'admin';
    }

    isModerator() {
        return this.role === 'moderator' || this.isAdmin();
    }

    hasPermission(permission) {
        const permissions = {
            'user': ['read'],
            'moderator': ['read', 'write'],
            'admin': ['read', 'write', 'delete', 'manage']
        };

        return permissions[this.role]?.includes(permission) || false;
    }

    toJSON() {
        return {
            id: this.id,
            name: this.name,
            email: this.email,
            role: this.role,
            createdAt: this.createdAt,
            updatedAt: this.updatedAt
        };
    }

    static fromJSON(json) {
        return new User(JSON.parse(json));
    }

    validate() {
        const errors = [];

        if (!this.name || this.name.length < 2) {
            errors.push('Name must be at least 2 characters');
        }

        if (!this.email || !this.email.includes('@')) {
            errors.push('Valid email is required');
        }

        if (!['user', 'moderator', 'admin'].includes(this.role)) {
            errors.push('Invalid role');
        }

        return {
            isValid: errors.length === 0,
            errors
        };
    }
}