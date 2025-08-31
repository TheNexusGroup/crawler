import { User } from '../models/User.js';
import { Validator } from '../utils/Validator.js';
import { CacheManager } from '../utils/CacheManager.js';

export class UserService {
    constructor(database) {
        this.db = database;
        this.validator = new Validator();
        this.cache = new CacheManager();
    }

    async getAllUsers() {
        const cacheKey = 'all_users';
        let users = this.cache.get(cacheKey);
        
        if (!users) {
            users = await this.db.query('SELECT * FROM users');
            this.cache.set(cacheKey, users, 300); // Cache for 5 minutes
        }
        
        return users.map(userData => new User(userData));
    }

    async getUserById(id) {
        if (!this.validator.isValidId(id)) {
            throw new Error('Invalid user ID');
        }

        const cacheKey = `user_${id}`;
        let userData = this.cache.get(cacheKey);

        if (!userData) {
            userData = await this.db.query('SELECT * FROM users WHERE id = ?', [id]);
            if (!userData) {
                return null;
            }
            this.cache.set(cacheKey, userData, 300);
        }

        return new User(userData);
    }

    async createUser(userData) {
        const validation = this.validator.validateUserData(userData);
        if (!validation.isValid) {
            throw new Error(`Validation failed: ${validation.errors.join(', ')}`);
        }

        const user = new User(userData);
        const result = await this.db.query(
            'INSERT INTO users (name, email, role) VALUES (?, ?, ?)',
            [user.name, user.email, user.role]
        );

        user.id = result.insertId;
        
        // Clear cache
        this.cache.clear('all_users');
        
        return user;
    }

    async updateUser(id, updateData) {
        const existingUser = await this.getUserById(id);
        if (!existingUser) {
            throw new Error('User not found');
        }

        const validation = this.validator.validateUserData(updateData, true);
        if (!validation.isValid) {
            throw new Error(`Validation failed: ${validation.errors.join(', ')}`);
        }

        await this.db.query(
            'UPDATE users SET name = ?, email = ?, role = ? WHERE id = ?',
            [updateData.name, updateData.email, updateData.role, id]
        );

        // Clear cache
        this.cache.clear(`user_${id}`);
        this.cache.clear('all_users');

        return this.getUserById(id);
    }

    async deleteUser(id) {
        const user = await this.getUserById(id);
        if (!user) {
            throw new Error('User not found');
        }

        await this.db.query('DELETE FROM users WHERE id = ?', [id]);
        
        // Clear cache
        this.cache.clear(`user_${id}`);
        this.cache.clear('all_users');

        return true;
    }
}