#ifndef __FAIMTEST_H__
#define __FAIMTEST_H__

#include <aim.h> 

extern int keepgoing;
extern struct aim_session_t aimsess;

int login(const char *sn, const char *passwd);
int logout(void);

void cmd_init(void);
void cmd_gotkey(void);
void cmd_uninit(void);

#endif /* __FAIMTEST_H__ */
