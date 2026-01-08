/*
 * cli.c
 *
 *  Created on: 2026年1月6日
 *      Author: SYRLIST
 */
#include "cli.h"
#include "platform_uart.h"
#include <string.h>
#include <stdio.h>

#ifndef CLI_LINE_MAX
#define CLI_LINE_MAX 64
#endif

#ifndef CLI_CMD_MAX
#define CLI_CMD_MAX 16
#endif

typedef struct {
    const char   *name;
    const char   *help;
    cli_cmd_fn_t  fn;
} cli_cmd_t;

static cli_cmd_t s_cmds[CLI_CMD_MAX];
static int       s_cmd_cnt = 0;

static char   s_line[CLI_LINE_MAX];
static size_t s_len = 0;

static void cli_puts(const char *s)
{
    if (!s) return;
    platform_uart_write((const uint8_t*)s, strlen(s));
}

static void cli_prompt(void)
{
    cli_puts("\r\n> ");
}

static void cli_exec_line(char *line)
{
    // tokenize by spaces
    char *argv[8] = {0};
    int argc = 0;

    char *p = line;
    while (*p && argc < (int)(sizeof(argv) / sizeof(argv[0]))) {
        while (*p == ' ') p++;
        if (*p == '\0') break;

        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    if (argc == 0) return;

    for (int i = 0; i < s_cmd_cnt; i++) {
        if (strcmp(argv[0], s_cmds[i].name) == 0) {
            s_cmds[i].fn(argc, argv);
            return;
        }
    }

    cli_puts("\r\nunknown cmd. type 'help'.");
}

void cli_init(void)
{
    s_cmd_cnt = 0;
    s_len = 0;
    memset(s_line, 0, sizeof(s_line));
    cli_puts("\r\nCLI ready. type 'help'.");
    cli_prompt();
}

int cli_register(const char *name, const char *help, cli_cmd_fn_t fn)
{
    if (!name || !help || !fn) return -2;
    if (s_cmd_cnt >= CLI_CMD_MAX) return -1;

    s_cmds[s_cmd_cnt].name = name;
    s_cmds[s_cmd_cnt].help = help;
    s_cmds[s_cmd_cnt].fn   = fn;
    s_cmd_cnt++;
    return 0;
}

void cli_feed_byte(uint8_t b)
{
    // CR/LF -> execute
    if (b == '\r' || b == '\n') {
        s_line[s_len] = '\0';
        cli_exec_line(s_line);
        s_len = 0;
        memset(s_line, 0, sizeof(s_line));
        cli_prompt();
        return;
    }

    // backspace
    if (b == 0x08 || b == 0x7F) {
        if (s_len > 0) s_len--;
        return;
    }

    // printable
    if (b >= 32 && b <= 126) {
        if (s_len + 1 < CLI_LINE_MAX) {
            s_line[s_len++] = (char)b;
        }
    }
}



