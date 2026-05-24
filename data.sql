INSERT INTO users (login, email, first_name, last_name, password_hash) VALUES
('alex.ivanov', 'alex.ivanov@example.com', 'Alex', 'Ivanov', 'hash_01'),
('maria.petrova', 'maria.petrova@example.com', 'Maria', 'Petrova', 'hash_02'),
('john.smith', 'john.smith@example.com', 'John', 'Smith', 'hash_03'),
('anna.kuznetsova', 'anna.kuznetsova@example.com', 'Anna', 'Kuznetsova', 'hash_04'),
('pavel.sidorov', 'pavel.sidorov@example.com', 'Pavel', 'Sidorov', 'hash_05'),
('elena.volkova', 'elena.volkova@example.com', 'Elena', 'Volkova', 'hash_06'),
('sergey.orlov', 'sergey.orlov@example.com', 'Sergey', 'Orlov', 'hash_07'),
('kate.romanova', 'kate.romanova@example.com', 'Kate', 'Romanova', 'hash_08'),
('igor.lebedev', 'igor.lebedev@example.com', 'Igor', 'Lebedev', 'hash_09'),
('nina.fedorova', 'nina.fedorova@example.com', 'Nina', 'Fedorova', 'hash_10');

INSERT INTO folders (user_id, name) VALUES
(1, 'Inbox'),
(1, 'Sent'),
(2, 'Inbox'),
(2, 'Drafts'),
(3, 'Inbox'),
(4, 'Inbox'),
(5, 'Archive'),
(6, 'Inbox'),
(7, 'Work'),
(8, 'Personal');

INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent) VALUES
(1, 1, 'team@example.com', 'Sprint plan', 'Discuss sprint tasks for next week', TRUE),
(2, 1, 'boss@example.com', 'Status update', 'Weekly status report attached', TRUE),
(3, 2, 'friend@example.com', 'Weekend', 'Are we meeting on Saturday?', FALSE),
(4, 2, 'hr@example.com', 'Draft response', 'Prepared answer for HR request', FALSE),
(5, 3, 'client@example.com', 'Invoice', 'Please review invoice #45', TRUE),
(6, 4, 'support@example.com', 'Bug report', 'Found an issue in the mobile app', TRUE),
(7, 5, 'archive@example.com', 'Old thread', 'Saved message for archive', FALSE),
(8, 6, 'manager@example.com', 'Standup notes', 'Today I finished API tests', TRUE),
(9, 7, 'partner@example.com', 'Contract', 'Need to review the contract draft', TRUE),
(10, 8, 'travel@example.com', 'Tickets', 'Your tickets are attached', TRUE);
