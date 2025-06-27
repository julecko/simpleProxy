#ifndef DB_USER_H
#define DB_USER_H

char *db_user_migration();
char *db_user_add(const char *name, const char *base64_pass);
char *db_user_add_encrypt(const char *name, const char *pass);

#endif