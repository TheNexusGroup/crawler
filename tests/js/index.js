import express from 'express';
import { UserService } from './services/UserService.js';
import { AuthMiddleware } from './middleware/AuthMiddleware.js';
import { DatabaseConnection } from './db/Connection.js';
import { Logger } from './utils/Logger.js';

const app = express();
const logger = new Logger('Main');
const db = new DatabaseConnection();
const userService = new UserService(db);
const authMiddleware = new AuthMiddleware(userService);

app.use(express.json());
app.use(authMiddleware.authenticate);

app.get('/users', async (req, res) => {
    try {
        const users = await userService.getAllUsers();
        res.json(users);
    } catch (error) {
        logger.error('Failed to get users:', error);
        res.status(500).json({ error: 'Internal server error' });
    }
});

app.post('/users', async (req, res) => {
    try {
        const user = await userService.createUser(req.body);
        res.status(201).json(user);
    } catch (error) {
        logger.error('Failed to create user:', error);
        res.status(400).json({ error: error.message });
    }
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    logger.info(`Server running on port ${PORT}`);
});

export default app;