import { Logger } from '../utils/Logger.js';
import { TokenManager } from '../utils/TokenManager.js';

export class AuthMiddleware {
    constructor(userService) {
        this.userService = userService;
        this.logger = new Logger('AuthMiddleware');
        this.tokenManager = new TokenManager();
    }

    authenticate = async (req, res, next) => {
        try {
            const token = this.extractToken(req);
            
            if (!token) {
                return res.status(401).json({ error: 'Authentication token required' });
            }

            const decoded = this.tokenManager.verify(token);
            if (!decoded) {
                return res.status(401).json({ error: 'Invalid token' });
            }

            const user = await this.userService.getUserById(decoded.userId);
            if (!user) {
                return res.status(401).json({ error: 'User not found' });
            }

            req.user = user;
            req.token = token;
            
            this.logger.debug(`Authenticated user: ${user.email}`);
            next();
        } catch (error) {
            this.logger.error('Authentication error:', error);
            res.status(500).json({ error: 'Authentication failed' });
        }
    };

    requireRole = (requiredRole) => {
        return (req, res, next) => {
            if (!req.user) {
                return res.status(401).json({ error: 'Authentication required' });
            }

            if (!this.hasRequiredRole(req.user.role, requiredRole)) {
                return res.status(403).json({ error: 'Insufficient permissions' });
            }

            next();
        };
    };

    requirePermission = (permission) => {
        return (req, res, next) => {
            if (!req.user) {
                return res.status(401).json({ error: 'Authentication required' });
            }

            if (!req.user.hasPermission(permission)) {
                return res.status(403).json({ error: `Permission '${permission}' required` });
            }

            next();
        };
    };

    extractToken(req) {
        const authHeader = req.headers.authorization;
        
        if (authHeader && authHeader.startsWith('Bearer ')) {
            return authHeader.slice(7);
        }
        
        return req.query.token || req.body.token || null;
    }

    hasRequiredRole(userRole, requiredRole) {
        const roleHierarchy = {
            'user': 1,
            'moderator': 2,
            'admin': 3
        };

        return roleHierarchy[userRole] >= roleHierarchy[requiredRole];
    }
}